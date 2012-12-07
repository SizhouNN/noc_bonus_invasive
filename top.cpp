#include <time.h>
#include <systemc.h>

#include "icore.h"
#include "pe.h"

SC_MODULE(top)
{
public:
	enum {N = 2};
	enum {M = 2};

	router *routers[N][M];
	PE_base *pes[N][M];

	sc_signal<packet> router_to_pe[N][M], pe_to_router[N][M];
	sc_signal<packet> router_to_router_east[N][M-1], router_to_router_west[N][M-1];
	sc_signal<packet> router_to_router_north[N-1][M], router_to_router_south[N-1][M];
	sc_signal<packet> terminal_loop_north[M], terminal_loop_south[M];
	sc_signal<packet> terminal_loop_east[N], terminal_loop_west[N];
	sc_signal<bool> clock;

	SC_CTOR(top)
	{
		create_pes();
		create_network();
	}

protected:
	void create_pes()
	{
		for (int i = 0; i < N; ++i)
		{

			for (int j = 0; j < M; ++j)
			{	

				char name[100];
				sprintf(name, "PE%d_%d", j, i);
				pes[i][j] = new PE_unit(name);
				pes[i][j]->clock(clock);
				pes[i][j]->set_xy(j, i);
				pes[i][j]->init();

			}
		}
		

	}

	void create_network()
	{
		for (int i = 0; i < N; ++i)//define i by N row vertical
		{
			for (int j = 0; j < M; ++j) //define j by M column horizontal
			{
				char name[100];
				sprintf(name, "router%d_%d", j, i);

				// create router
				routers[i][j] = new router(name);
				routers[i][j]->set_xy(j,i);
				routers[i][j]->clock(clock);
				routers[i][j]->init();

				if(i == 0)
				{
					routers[i][j]->port_in[router::NORTH](
						terminal_loop_north[j]);
					routers[i][j]->port_out[router::NORTH](
						terminal_loop_north[j]);
				}
				else //connect to NORTH
				{
					routers[i][j]->port_out[router::NORTH](
						router_to_router_north[i-1][j]);
					routers[i][j]->port_in[router::NORTH](
						router_to_router_south[i-1][j]);
				}

				if(i == N-1)
				{
					routers[i][j]->port_in[router::SOUTH](
						terminal_loop_south[j]);
					routers[i][j]->port_out[router::SOUTH](
						terminal_loop_south[j]);
				}
				else //connect to SOUTH
				{
					routers[i][j]->port_out[router::SOUTH](
						router_to_router_south[i][j]);
					routers[i][j]->port_in[router::SOUTH](
						router_to_router_north[i][j]);
				}

				// connect router to west routers
				if (j != 0)
				{
					routers[i][j]->port_out[router::WEST](
						router_to_router_west[i][j-1]);
					routers[i][j]->port_in[router::WEST](
						router_to_router_east[i][j-1]);
				}
				else // or make a loop
				{
					routers[i][j]->port_out[router::WEST](
						terminal_loop_west[i]);
					routers[i][j]->port_in[router::WEST](
						terminal_loop_west[i]);
				}

				if (j != M-1) // connect router to east routers
				{
					routers[i][j]->port_out[router::EAST](
						router_to_router_east[i][j]);
					routers[i][j]->port_in[router::EAST](
						router_to_router_west[i][j]);
				}
				else // or make a loop
				{
					routers[i][j]->port_out[router::EAST](
						terminal_loop_east[i]);
					routers[i][j]->port_in[router::EAST](
						terminal_loop_east[i]);
				}

				// connect router to PE
				routers[i][j]->port_out[router::PE](router_to_pe[i][j]);
				routers[i][j]->port_in[router::PE](pe_to_router[i][j]);
				pes[i][j]->data_in(router_to_pe[i][j]);
				pes[i][j]->data_out(pe_to_router[i][j]);
			}
		}

		routers[0][0]->load(1000);
	}

}; // top

int sc_main(int argc , char *argv[])
{
	srand(0);

	top top_module("top");
	int runtime;

	//sc_trace_file *tf = sc_create_vcd_trace_file("TRACEFILE");
	//sc_trace(tf, top_module.pe_to_router[0][0], "pe_to_router00");
	//sc_trace(tf, top_module.clock, "clock");
	//Banner
	printf("*********************************************\n");
	printf("*             ECE 587  Fall 2012            *\n");
	printf("*                  Project 2                *\n");
	printf("*                                           *\n");
	printf("*    Sizhou Wang A20249772                  *\n");
	printf("*    Thank You !                            *\n");
	printf("*********************************************\n");

	printf("\nPlease enter the number of cycles to run >>>");
	scanf("%d", &runtime);

	printf("cycle  0 ================================\n");
	sc_start(0, SC_NS);

	for(int i = 1; i < runtime; i++){

		printf("cycle %2d ================================\n", i);

		top_module.clock.write(1);
		sc_start(10, SC_NS);
		top_module.clock.write(0);
		sc_start(10, SC_NS);
	}

	return 0;
}
