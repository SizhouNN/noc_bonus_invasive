#include "icore.h"

std::ostream &operator<<(std::ostream &o, const packet &p)
{
	char buf[100];
	sprintf(buf, "(%d,%d)->(%d,%d)",
		p.src_x, p.src_y, p.dest_x, p.dest_y);
	return o << buf << ", " << p.token;
}

void sc_trace(sc_trace_file *tf, const packet &p, const std::string &name)
{
	sc_trace(tf, p.src_x, name+".src.x");
	sc_trace(tf, p.src_y, name+".src.y");
	sc_trace(tf, p.dest_x, name+".dest.x");
	sc_trace(tf, p.dest_y, name+".dest.y");
	sc_trace(tf, p.token, name+".token");
}

void router::main()
{
	assert((x_ != -1) && (y_ != -1)); // to identify PE

	//#-1 Read PE port
	read_packet_PE();


	//#-2 read other port
	for (int iport = 1; iport < PORTS; ++iport)
		read_packet(iport);

	printf("icore%d_%d: %d done\n", x_, y_, tasksdone);

	schedule();

	for (int iport = 0; iport < PORTS; ++iport)
		write_packet(iport);
}

void router::set_xy(int x, int y)
{
	assert((x_ == -1) && (y_ == -1)); // set once only
	assert((x != -1) && (y != -1)); // must use a legal location

	x_ = x;
	y_ = y;
}

void router::read_packet(int iport)
{
	assert(iport < PORTS);

	packet p = port_in[iport].read();
	//printf("%d_%d: %d\n", x_, y_, p.msgType);

	if ((p.src_x == -1) && (p.src_y == -1))
	{

		return; 
	}
	//####################################
	if(p.msgType == RESULT)
	{

		if((p.task.taskOwner.x == x_) && (p.task.taskOwner.y == y_))
		{
			//mine finished task, update
			int index = p.task.taskIndex;
			mem.mem_status[index] = DONE;
			mem.mem_tasks[index] = p.task;
		}
		else
		{
			//not my task route it out
			route_packet_xy(p);
		}
	}
	//####################################
	else if(p.msgType == INQUIRY)
	{
		//printf("##############");
		//return if the packet is mine, meaning its a loop

		if((p.task.taskOwner.x == x_) && (p.task.taskOwner.y == y_))
		{
			int i;
			for(i=0;i<MEM_SIZE;i++)
			{
				if(mem.mem_tasks[i] == p.task)
					break;
			}
			mem.mem_status[i] = TODO;
			deadloop[iport] = 1;
		}
		else //not mine meaning a new job
		{
			if(mem.mem_num<MEM_SIZE)
			{
				//get the task and return accept

				int j = 0;
				for(j=0;j<MEM_SIZE;j++)
				{
					if(mem.mem_identity[j] == NONE)
						break;
				}
				if (j < 5000)
				{
					mem.mem_tasks[j] = p.task;
					mem.mem_status[j] = TODO;
					mem.mem_identity[j] = NOTMINE;
					mem.mem_num++;
					//prepare a ACCEPT

					packet accept = packet();
					accept.msgType = ACCEPT;
					accept.dest_x = p.src_x;
					accept.dest_y = p.src_y;
					accept.src_x = x_;
					accept.src_y = y_;
					accept.task = p.task;

					route_packet_xy(accept);
				}
				else if (j == 5000)
				{
					//prepare a OCCUPY 
					packet occupy = packet();
					occupy.msgType = OCCUPY;
					occupy.dest_x = p.src_x;
					occupy.dest_y = p.src_y;
					occupy.src_x = x_;
					occupy.src_y = y_;
					occupy.task = p.task;

					route_packet_xy(occupy);
				}
			}
			else //mem full
			{
				//prepare a OCCUPY 
				packet occupy = packet();
				occupy.msgType = OCCUPY;
				occupy.dest_x = p.src_x;
				occupy.dest_y = p.src_y;
				occupy.src_x = x_;
				occupy.src_y = y_;
				occupy.task = p.task;

				route_packet_xy(occupy);
			}
		}
	}
	//####################################
	else if(p.msgType == OCCUPY)
	{
		//search and status==todo
		int i;
		for(i=0;i<MEM_SIZE;i++)
		{
			if(mem.mem_tasks[i] == p.task)
				break;
		}
		mem.mem_status[i] = TODO;
	}
	//####################################
	else if(p.msgType == ACCEPT)
	{
		if((p.task.taskOwner.x == x_) && (p.task.taskOwner.y == y_))
		{
			//mine inquiry task, update
			int index = p.task.taskIndex;
			mem.mem_status[index] = ACCEPTED;
		}
		else
		{
			//not my task delete mem

			int i;
			for(i=0;i<MEM_SIZE;i++)
			{
				if(mem.mem_tasks[i] == p.task)
					break;
			}
			mem.mem_identity[i] = NONE;
			mem.mem_status[i] = DONE;
			mem.mem_num--;
		}
	}


}

void router::write_packet(int iport)
{
	assert(iport < PORTS);

	if (out_queue_[iport].empty())
	{
		port_out[iport].write(packet()); // write an empty packet
		//printf("#########\n");
	}
	else
	{
		port_out[iport].write(out_queue_[iport].front());
		out_queue_[iport].pop_front();
	}
}

void router::route_packet_xy(packet p)
{
	//printf("#%d, %d#\n", p.dest_x, p.dest_y);
	if ((p.dest_x == -1) || (p.dest_y == -1))
	{   //printf("################################");
		printf("router (%d,%d): drop packet with invalid destination"
			" (%d,%d)->(%d,%d)\n",
			p.src_x, p.src_y, p.dest_x, p.dest_y);
		return;
	}

	// ignore dest_y for now
	if (p.dest_y == y_)
	{
		if (p.dest_x == x_) // to PE
		{
			out_queue_[PE].push_back(p);
		}
		else if (p.dest_x < x_) // left to WEST
		{
			out_queue_[WEST].push_back(p);
		}
		else // (p.dest_x > x_) right to EAST
		{
			out_queue_[EAST].push_back(p);
		}
	}
	else if (p.dest_y < y_) //up to NORTH
	{
		out_queue_[NORTH].push_back(p);
	}
	else //(p.dest_y > y_) down to SOUTH
	{
		out_queue_[SOUTH].push_back(p);
	}
}

void router::load(int MAX)
{
	int task_num = MAX/20;
	//printf("%d ###########", task_num);
	//generate same task of same size
	task_type tasks = task_type();

	for (int i = 0; i<task_num;i++)
	{
		mem.mem_identity[i] = MINE;
		mem.mem_num++;
		mem.mem_status[i] = TODO;

		tasks.pc = 0;
		tasks.taskIndex = i;
		tasks.taskOwner.x = x_;
		tasks.taskOwner.x = y_;

		mem.mem_tasks[i] = tasks;
	}
	//printf("MEM: %d, %d\n", mem.mem_status[2], mem.mem_identity[2]);
}

void router::read_packet_PE()
{
	packet pe = port_in[0].read();
	if((pe.src_x == -1) && (pe.src_y == -1))
		return;


	if (pe.msgType == OCCUPY || pe.msgType == OVERHEAT)
	{
		//printf("BUSY\n");
		busy = 1;
	}
	else if(pe.msgType == RESULT)
	{
		//printf("##########\n");
		busy = 0;
		if((pe.task.taskOwner.x == x_) && (pe.task.taskOwner.y == y_))
		{
			//mine finished task, update
			int index = pe.task.taskIndex;
			mem.mem_status[index] = DONE;
			mem.mem_tasks[index] = pe.task;
			tasksdone++;

		}
		else
		{
			//not my task route it out and delete mem

			route_packet_xy(pe);

			int i;
			for(i=0;i<MEM_SIZE;i++)
			{
				if(mem.mem_tasks[i] == pe.task)
					break;
			}
			mem.mem_identity[i] = NONE;
			mem.mem_status[i] = DONE;
			mem.mem_num--;
			tasksdone++;
			//printf("###############\n");
		}
	}
}

void router::schedule()
{
	//PE
	if(busy == 0)
	{
		//prepare a PE 
		for (int i = 0;i<MEM_SIZE;i++)
		{

			if((mem.mem_identity[i] != NONE) && (mem.mem_status[i] == TODO))
			{
				packet inquiry_pe = packet();
				inquiry_pe.msgType = INQUIRY;
				inquiry_pe.src_x = x_;
				inquiry_pe.src_y = y_;
				inquiry_pe.task = mem.mem_tasks[i];
				inquiry_pe.dest_x = x_;
				inquiry_pe.dest_y = y_;
				//handle local mem
				mem.mem_status[i] = PENDING;
				//printf("%d_%d SENDOUT: %d\n", x_, y_, inquiry_pe.msgType);
				//send out
				out_queue_[0].push_back(inquiry_pe);
				break;
			}
		}
	}

	//Other
	for (int iport = 1; iport < PORTS; ++iport)
	{
		if (deadloop[iport] != 1)
		{
			for (int i = 0;i<MEM_SIZE;i++)
			{
				if((mem.mem_identity[i] != NONE) && (mem.mem_status[i] == TODO))
				{
					//prepare a INQUIRY 
					packet inquiry = packet();
					inquiry.msgType = INQUIRY;
					inquiry.src_x = x_;
					inquiry.src_y = y_;
					inquiry.task = mem.mem_tasks[i];
					inquiry.dest_x = -1;//no need to care about dest.
					inquiry.dest_y = -1;
					//handle local mem
					mem.mem_status[i] = PENDING;
					//send out
					out_queue_[iport].push_back(inquiry);
					break;
				}
			}
		}
		else if(deadloop[iport] == 1)
		{
			out_queue_[iport].push_back(packet());
		}
	}
}

void router::init()
{
	tasksdone = 0;
	busy = 0;
}