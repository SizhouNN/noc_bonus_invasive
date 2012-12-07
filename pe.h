#ifndef PE_H
#define PE_H

#include "icore.h"

SC_MODULE(PE_base)
{
	sc_in<bool> clock; // port for the global clock

	sc_in<packet> data_in; // from router
	sc_out<packet> data_out; // to router

	void main() // specify the functionality of PE per clock cycle
	{
		read_input();
		execute();
		write_output();
	}

	SC_CTOR(PE_base)
		: x_(-1), y_(-1)
	{
		SC_METHOD(main);
		sensitive << clock.pos();
	}

	// use this function to set the coordinates of the PE
	void set_xy(int x, int y);

	virtual ~PE_base() {}
	
protected:
	std::list<packet> out_queue_; // output queue
	packet packet_in_; // incoming packet from the router

	int x_, y_; // location of the PE

	virtual void read_input(); // read a packet from the the router
	virtual void execute() = 0; // abstraction of computations
	virtual void write_output(); // // send a packet to the router
public:
	virtual void init() = 0;
	

}; // PE_base



class PE_unit : public PE_base
{
public:
	PE_unit(const char *name) : PE_base(name) {}
	int overheat;
	void init();
protected:
	int occupy; //0 for no, 1 for yes

	
	//int PC; //program counter from 0 to 19
	task_type RAM;

	void ALU();
	void Controller();
	
	void execute();


};

#endif // PE_H
