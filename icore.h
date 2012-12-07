#ifndef ROUTER_H
#define ROUTER_H

#include <systemc.h>
#include <list>

#define MEM_SIZE 5000
// a empty packet has src_x == src_y == dest_x == dest_y == -1
typedef int token_type;

enum task_identity_type {NONE, MINE, NOTMINE};
enum task_status_type {ACCEPTED, PENDING, TODO, DONE};

enum msg_type{ INQUIRY, RESULT, OVERHEAT, ACCEPT, OCCUPY };

typedef struct dim2
{
	int x;
	int y;

	dim2(): x(-1), y(-1)
	{
	}

	dim2(int, int);

	bool operator==(const dim2 &xx) const
	{
		return (xx.x == x) && (xx.y == y);
	}


}dim2;

typedef struct command_type
{
	int cmd;
	//struct of input
	//here just for simple
	int input;
	int output;
	//int finish;//0 for not finish, 1 for finished
	command_type(): cmd(0), input(-1), output(-1)
	{
	}


}command_type;

typedef struct task_type
{
	int pc; //0 to 19
	dim2 taskOwner;
	int taskIndex;
	
	command_type command[20];

	task_type(dim2 dim2t = dim2()): taskOwner(dim2t), taskIndex(-1), pc(-1)
	{
	}

	bool operator==(const task_type &x) const
	{
		return (x.taskIndex == taskIndex) && (x.taskOwner == taskOwner) && (x.pc == pc);
	}

}task_type;

typedef struct mem_type
{
	int mem_num;//existing task number; there is no file system so just loop 5000 time to look for the mem_num number of tasks
	enum task_identity_type mem_identity[MEM_SIZE];
	task_type mem_tasks[MEM_SIZE];
	enum task_status_type mem_status[MEM_SIZE];

}mem_type;

struct packet
{
	int src_x, src_y;
	int dest_x, dest_y;
	token_type token;

	enum msg_type msgType;
	task_type task;




	packet(int sx = -1, int sy = -1, int dx = -1, int dy = -1,
		token_type t = token_type())
		: src_x(sx), src_y(sy), dest_x(dx), dest_y(dy), token(t)
	{
	}

	bool operator==(const packet &x) const
	{
		return (x.src_x == src_x) && (x.src_y == src_y)
			&& (x.dest_x == dest_x) && (x.dest_y == dest_y)
			&& (x.token == token) && (x.task == task) && (x.msgType == msgType);
	}
}; // struct packet

std::ostream &operator<<(std::ostream &o, const packet &p);
void sc_trace(sc_trace_file *tf, const packet &p, const std::string &name);

SC_MODULE(router)
{
	sc_in<bool> clock; // port for the global clock

	enum {PE = 0, NORTH, SOUTH, EAST, WEST, PORTS};
	sc_in<packet> port_in[PORTS]; // input ports
	sc_out<packet> port_out[PORTS]; // output ports

	void main(); // specify the functionality of router per clock cycle

	SC_CTOR(router)
		: x_(-1), y_(-1), key(0)
	{
		SC_METHOD(main);
		sensitive << clock.pos();
	}

	// use this function to set the coordinates of the router
	void set_xy(int x, int y);
	void load(int);
	void init();

protected:
	std::list<packet> out_queue_[PORTS]; // output queues

	int x_, y_; // location of the router

	void read_packet(int iport); // read a packet from the link
	void write_packet(int iport); // write a packet to the link
	void route_packet_xy(packet p); // route the packet to the output queue
	void read_packet_PE();

	int key; //indicates if top module assign tasks to this icore
	int busy;//0 is no, 1 is yes
	mem_type mem;
	int tasksdone;
	int deadloop[5];

	void schedule();
	
	

}; // router

#endif // ROUTER_H
