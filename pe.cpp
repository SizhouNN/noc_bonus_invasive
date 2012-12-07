#include "pe.h"

void PE_base::set_xy(int x, int y)
{
	assert((x_ == -1) && (y_ == -1)); // set once only
	assert((x != -1) && (y != -1)); // must use a legal location

	x_ = x;
	y_ = y;
}

void PE_base::read_input()
{
	packet_in_ = data_in.read();
	//if(packet_in_.msgType==INQUIRY) printf("%d INQUIRY\n", packet_in_.msgType);
}

void PE_base::write_output()
{
	if (out_queue_.empty())
	{
		data_out.write(packet());
	}
	else
	{
		data_out.write(out_queue_.front());
		out_queue_.pop_front();
	}
}





void PE_unit::ALU()
{
	int localpc = RAM.pc;
	int tmp = RAM.command[localpc].input;
	RAM.command[localpc].output = tmp + 10000;
	RAM.pc++;

	return;
}

void PE_unit::execute()
{

	packet return_p = packet();
	//#-1 
	//If overheat, return over heat and return from execute immediately, abandon task
	if (overheat == 1)
	{
		return_p.dest_x = x_;
		return_p.dest_y = y_;
		return_p.src_x = x_;
		return_p.src_y = y_;

		return_p.msgType = OVERHEAT;

		return_p.task = task_type();
		out_queue_.push_back(return_p);
		return;
	}

	//#-2 not overheat load RAM
	//input must not be Result or Overheat
	//printf("%d INQUIRY\n", packet_in_.msgType);
	if ((packet_in_.src_x != -1) && (packet_in_.src_y != -1) && (packet_in_.msgType == INQUIRY)) // if incomming new job
	{

		if (occupy == 1)
		{
			//return occupy msg
			return_p.dest_x = x_;
			return_p.dest_y = y_;
			return_p.src_x = x_;
			return_p.src_y = y_;

			return_p.msgType = OCCUPY;
			return_p.task = RAM; //occupied by current RAM content

		}
		else if (occupy == 0)//free
		{

			RAM = packet_in_.task;
			occupy = 1;
			
		}
	}
	//printf("%d, pc, %d\n", RAM.pc, occupy);

	//#-3 RAM is loaded run ALU and try to finish it
	if ((RAM.pc < 20) && (occupy == 1))
	{
		
		ALU();
	}
	
	if (RAM.pc = 20 && occupy ==1)//job done
	{
		//return result msg
		return_p.dest_x = x_;
		return_p.dest_y = y_;
		return_p.src_x = x_;
		return_p.src_y = y_;
		return_p.msgType = RESULT;
		return_p.task = RAM; //result from current RAM content
		occupy = 0;
		
	}

	//#-4 push to output queue
	out_queue_.push_back(return_p);

}

void PE_unit::init()
{
	overheat = 0;
	occupy = 0;
}