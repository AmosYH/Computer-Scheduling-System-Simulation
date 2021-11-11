#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
using namespace std;

#define MAX_LOOP 1000
#define QUANTAM_TIME 6
struct service_t{
	string type;	// C (cpu), D (disk)
	int time_cost;
	service_t(): type(""), time_cost(-1) {}
	service_t(string type, string desc): type(type) { 
                if ( desc == "mtx" )
                {
                        this->time_cost = 0;
                        return;
                }
		this->time_cost = stoi(desc);
	}
};

struct process_t
{
	int process_id;
	int arrival_time;
	vector<service_t> service_seq;
	service_t cur_service;
	int cur_service_idx;
	int cur_service_tick;	// num of ticks that has been spent on current service
	vector<int> working;	// working sequence on CPU, for loging output
        int cur_RQ;

	// Call when current service completed
	// if there are no service left, return true. Otherwise, return false
	bool proceed_to_next_service()
	{
		this->cur_service_idx++;
		this->cur_service_tick = 0;
		if(this->cur_service_idx >= this->service_seq.size()) {	// all services are done, process should end
			return true;
		}
		else {		// still requests services
			this->cur_service = this->service_seq[this->cur_service_idx];
			return false;
		}
	};

	// Log the working ticks on CPU (from 'start_tick' to 'end_tick')
	void log_working(int start_tick, int end_tick)
	{
		this->working.push_back(start_tick);
		this->working.push_back(end_tick);
	};
};
bool processIdAsc(const process_t &p1, const process_t &p2)
{
    return p1.process_id < p2.process_id;
}
// write output log
int write_file(vector<process_t> processes, const char* file_path)
{
    ofstream outputfile;
    outputfile.open(file_path);
    for (vector<process_t>::iterator p_iter = processes.begin(); p_iter != processes.end(); p_iter++) {
    	outputfile << "process " << p_iter->process_id << endl; 
      	for (vector<int>::iterator w_iter = p_iter->working.begin(); w_iter != p_iter->working.end(); w_iter++) {
        	outputfile << *w_iter << " ";
      	}
      	outputfile << endl;
    }
    outputfile.close();
    return 0;
}

// Split a string according to a delimiter
void split(const string& s, vector<string>& tokens, const string& delim=" ") 
{
	string::size_type last_pos = s.find_first_not_of(delim, 0); 
	string::size_type pos = s.find_first_of(delim, last_pos);
	while(string::npos != pos || string::npos != last_pos) {
		tokens.push_back(s.substr(last_pos, pos - last_pos));
		last_pos = s.find_first_not_of(delim, pos); 
		pos = s.find_first_of(delim, last_pos);
	}
}

vector<process_t> read_processes(const char* file_path)
{
	vector<process_t> process_queue;
	ifstream file(file_path);
	string str;
	while(getline(file, str)) {
		process_t new_process;
		stringstream ss(str);
		int service_num;
		char syntax;
		ss >> syntax >> new_process.process_id >> new_process.arrival_time >> service_num;
		for(int i = 0; i < service_num; i++) {	// read services sequence
			getline(file, str);
			str = str.erase(str.find_last_not_of(" \n\r\t") + 1);
			vector<string> tokens;
			split(str, tokens, " ");
			service_t ser(tokens[0], tokens[1]);
			new_process.service_seq.push_back(ser);
		}
		new_process.cur_service_idx = 0;
		new_process.cur_service_tick = 0;
                new_process.cur_RQ = 0;
		new_process.cur_service = new_process.service_seq[new_process.cur_service_idx];
		process_queue.push_back(new_process);
	}
	return process_queue;
}
bool mutex_lock(bool &mutex_acquired)
{
        if ( mutex_acquired == false )
        {
                mutex_acquired = true;
                return true;
        }
        return false;
}
bool mutex_unlock(bool &mutex_acquired)
{
        mutex_acquired = false;
        return true;
}
// move the process at the front of q1 to the back of q2 (q1 head -> q2 tail)
int move_process_from(vector<process_t>& q1, vector<process_t>& q2)
{
	if(!q1.empty()) {
		process_t& tmp = q1.front();
		q2.push_back(tmp);
		q1.erase(q1.begin());
		return 1;
	}
	return 0;
}
void ReadyQueueDealer(int &cur_process_id, int &prev_process_id, vector<process_t> &ready_queue, vector<process_t> &block_queue, 
vector<process_t> &keyboard_queue, vector<process_t> &processes_done, vector<process_t> &mutex_queue, bool &mutex_acquired, 
int &quantum, int &dispatched_tick, int &cur_tick, int &complete_num)
{
        process_t& cur_process = ready_queue.front();	// dispatch the first process in ready queue
	cur_process_id = cur_process.process_id;
	if(cur_process_id != prev_process_id) {		// store the tick when current process is dispatched
		dispatched_tick = cur_tick;
	}
	cur_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
        quantum++;
	if(cur_process.cur_service_tick >= cur_process.cur_service.time_cost) {	// current service is completed
		bool process_completed = cur_process.proceed_to_next_service();
		if(process_completed) {		// the whole process is completed
                        cout << ready_queue.front().process_id << " process completed" << endl;
			complete_num++;
			cur_process.log_working(dispatched_tick, cur_tick + 1);
			move_process_from(ready_queue, processes_done);		// remove current process from ready queue
                        quantum = 1;
		}
                //Check if we unlock the mutex on next service, so we simply unlock and go to next service
                if ( cur_process.cur_service.type == "U" )
                {
                        mutex_unlock(mutex_acquired);
                        process_completed = cur_process.proceed_to_next_service();
                        if(process_completed) {		// the whole process is completed
                                complete_num++;
                                cur_process.log_working(dispatched_tick, cur_tick + 1);
                                move_process_from(ready_queue, processes_done);		// remove current process from ready queue
                        }
                        quantum = 1;
                }
                //Mutex lock required
                if ( cur_process.cur_service.type == "L" )
                {
                        //If lock is succesfully gained on first attempt, then we simply proceed to next service
                        if ( mutex_lock(mutex_acquired) == true)
                        {
                                cout << "Process " << ready_queue.front().process_id << " acquired lock" << endl;
                                cur_process.proceed_to_next_service();
                        }
                        //Else we add it to the mutex queue
                        else
                        {
                                cur_process.log_working(dispatched_tick, cur_tick + 1);
                                cur_process.proceed_to_next_service();
                                move_process_from(ready_queue, mutex_queue);
                                quantum = 1;
                        }
                }
		if(cur_process.cur_service.type == "D") {		// next service is disk I/O, block current process
			cur_process.log_working(dispatched_tick, cur_tick + 1);
			move_process_from(ready_queue, block_queue);
                        quantum = 1;
		}
                else if(cur_process.cur_service.type == "K") {		// next service is disk I/O, block current process
			cur_process.log_working(dispatched_tick, cur_tick + 1);
			move_process_from(ready_queue, keyboard_queue);
                        quantum = 1;
		}
        }        
}
int FB(vector<process_t> processes, const char* output_path) 
{
	vector<process_t> ready_queue;
        vector<process_t> ready1_queue;
        vector<process_t> ready2_queue;
	vector<process_t> block_queue;
        vector<process_t> mutex_queue;
        vector<process_t> keyboard_queue;
	vector<process_t> processes_done;
        vector<process_t> intermediate;
        bool readyQueueEmpty = false;
        bool ready1QueueEmpty = false;

	int complete_num = 0;
	int dispatched_tick = 0;
        int dispatched1_tick = 0;
        int dispatched2_tick = 0;
	int cur_process_id = -1;
        int prev_process_id = -1;
        int prev1_process_id = -1;
        int prev2_process_id = -1;
        bool mutex_acquired = false;
	int quantum = 1;
        int quantum1 = 1;
        int quantum2 = 1;
	
	for(int cur_tick = 0; cur_tick < MAX_LOOP; cur_tick++) 
        {
                readyQueueEmpty = false;
                ready1QueueEmpty = false;
                cout << "Time is: " << cur_tick << endl;
                if ( ready_queue.empty() )
                {
                        cout << "Ready queue is empty" << endl;
                }
                if ( ready1_queue.empty() )
                {
                        cout << "Ready1 queue is empty" << endl;
                }
                if ( ready2_queue.empty() )
                {
                        cout << "Ready2 queue is empty" << endl;
                } 
		// long term scheduler
		for(int i = 0; i < processes.size(); i++) {
			if(processes[i].arrival_time == cur_tick) {		// process arrives at current tick
				ready_queue.push_back(processes[i]);
                                quantum1 = 1;
                                quantum2 = 1;
			}
		}
                //Check if there is process mutex queue, add it to the ready queue and give it the lock
                if ( !mutex_queue.empty() )
                {
                        if ( mutex_lock(mutex_acquired) == true )
                        {
                                if ( mutex_queue[0].cur_RQ == 0 )
                                {
                                        move_process_from(mutex_queue,ready_queue);
                                        quantum1 = 1;
                                        quantum2 = 1;
                                }
                                else if ( mutex_queue[0].cur_RQ == 1 )
                                {
                                        move_process_from(mutex_queue,ready1_queue);
                                        quantum2 = 1;
                                }
                                else
                                {
                                        move_process_from(mutex_queue,ready2_queue);
                                }
                        }
                        
                }
		// O device scheduling
		if(!block_queue.empty()) {
			process_t& cur_io_process = block_queue.front();	// always provide service to the first process in block queue
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// I/O service is completed
				cur_io_process.proceed_to_next_service();
                                move_process_from(block_queue, intermediate);
			}
		}
                // I device scheduling
		if(!keyboard_queue.empty()) {
			process_t& cur_io_process = keyboard_queue.front();	// always provide service to the first process in block queue
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// I/O service is completed
				cur_io_process.proceed_to_next_service();
                                move_process_from(keyboard_queue, intermediate);
			}
		}
		// CPU scheduling
		if(ready_queue.empty()) {	// no process for scheduling
			prev_process_id = -1;	// reset the previous dispatched process ID to empty
                        readyQueueEmpty = true;
		}
		else {
			
                        ReadyQueueDealer(cur_process_id,prev_process_id,ready_queue,block_queue,keyboard_queue,processes_done,
                        mutex_queue,mutex_acquired,quantum,dispatched_tick,cur_tick,complete_num);
                        //Check if quantum time is completed
                        //If it has then re-set the quantum
                        process_t &cur_process = ready_queue.front();
                        if ( quantum >= QUANTAM_TIME )
                        {
                                cur_process.cur_RQ = 1;
                                cur_process.log_working(dispatched_tick, cur_tick + 1);
                                ready1_queue.push_back(ready_queue.front());
                                ready_queue.erase(ready_queue.begin());
                                dispatched_tick = cur_tick + 1;
                                quantum = 1;
                        }
			prev_process_id = cur_process_id;	// log the previous dispatched process ID
		}
                //1 ready queue
                // CPU scheduling
		if( readyQueueEmpty && ready1_queue.empty()) {	// no process for scheduling
			prev1_process_id = -1;	// reset the previous dispatched process ID to empty
                        ready1QueueEmpty = true;
		}
		else if ( readyQueueEmpty ){
			ReadyQueueDealer(cur_process_id,prev1_process_id,ready1_queue,block_queue,keyboard_queue,processes_done,
                        mutex_queue,mutex_acquired,quantum1,dispatched1_tick,cur_tick,complete_num);
                        process_t &cur_process = ready1_queue.front();
                        //Check if quantum1 time is completed
                        //If it has then re-set the quantum1
                        if ( quantum1 >= QUANTAM_TIME )
                        {
                                cur_process.cur_RQ = 2;
                                cur_process.log_working(dispatched1_tick, cur_tick + 1);
                                ready2_queue.push_back(ready1_queue.front());
                                ready1_queue.erase(ready1_queue.begin());
                                dispatched1_tick = cur_tick + 1;
                                quantum1 = 1;
                        }
			prev1_process_id = cur_process_id;	// log the previous dispatched process ID
		}
                //2 ready queue
                // CPU scheduling
		if( ready1QueueEmpty && ready2_queue.empty()) {	// no process for scheduling
			prev2_process_id = -1;	// reset the previous dispatched process ID to empty
		}
		else if ( ready1QueueEmpty ){
			ReadyQueueDealer(cur_process_id,prev2_process_id,ready2_queue,block_queue,keyboard_queue,processes_done,
                        mutex_queue,mutex_acquired,quantum2,dispatched2_tick,cur_tick,complete_num);
                        process_t &cur_process = ready2_queue.front();
                        //Check if quantum1 time is completed
                        //If it has then re-set the quantum1
                        if ( quantum2 >= QUANTAM_TIME )
                        {
                                cur_process.cur_RQ = 2;
                                cur_process.log_working(dispatched2_tick, cur_tick + 1);
                                ready2_queue.push_back(ready2_queue.front());
                                ready2_queue.erase(ready2_queue.begin());
                                dispatched2_tick = cur_tick + 1;
                                quantum2 = 1;
                        }
			prev2_process_id = cur_process_id;	// log the previous dispatched process ID
		}
                for ( int i = 0 ; i < intermediate.size() ; i++ )
                {
                        if ( intermediate[i].cur_RQ == 0 )
                        {
                                move_process_from(intermediate,ready_queue);
                                quantum1 = 1;
                                quantum2 = 1;
                        }
                        else if ( intermediate[i].cur_RQ == 1 )
                        {
                                move_process_from(intermediate,ready1_queue);
                                quantum2 = 1;
                        }
                        else
                        {
                                move_process_from(intermediate,ready2_queue);
                        }
                }
		if(complete_num == processes.size()) {	// all process completed
			break;
		}
	}
	write_file(processes_done, output_path);	// write output
	return 1;
}
int RR(vector<process_t> processes, const char* output_path) 
{
	vector<process_t> ready_queue;
	vector<process_t> block_queue;
        vector<process_t> mutex_queue;
        vector<process_t> keyboard_queue;
	vector<process_t> processes_done;
        vector<process_t> intermediate;

	int complete_num = 0;
	int dispatched_tick = 0;
	int cur_process_id = -1, prev_process_id = -1;
        bool mutex_acquired = false;
	int quantum = 1;

	for(int cur_tick = 0; cur_tick < MAX_LOOP; cur_tick++) {
		// long term scheduler
		for(int i = 0; i < processes.size(); i++) {
			if(processes[i].arrival_time == cur_tick) {		// process arrives at current tick
				ready_queue.push_back(processes[i]);
			}
		}
                //Check if there is process mutex queue, we add it to the ready queue and give it the lock
                if ( !mutex_queue.empty() )
                {
                        if ( mutex_lock(mutex_acquired) == true )
                        {
                                move_process_from(mutex_queue, ready_queue);
                        }
                        
                }
		// O device scheduling
		if(!block_queue.empty()) {
			process_t& cur_io_process = block_queue.front();	// always provide service to the first process in block queue
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// I/O service is completed
				cur_io_process.proceed_to_next_service();
                                move_process_from(block_queue, intermediate);
			}
		}
                // I device scheduling
		if(!keyboard_queue.empty()) {
			process_t& cur_io_process = keyboard_queue.front();	// always provide service to the first process in block queue
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// I/O service is completed
				cur_io_process.proceed_to_next_service();
                                move_process_from(keyboard_queue, intermediate);
			}
		}
		// CPU scheduling
		if(ready_queue.empty()) {	// no process for scheduling
			prev_process_id = -1;	// reset the previous dispatched process ID to empty
		}
		else {
			ReadyQueueDealer(cur_process_id,prev_process_id,ready_queue,block_queue,keyboard_queue,processes_done,
                        mutex_queue,mutex_acquired,quantum,dispatched_tick,cur_tick,complete_num);
                        process_t &cur_process = ready_queue.front();
                        //Check if quantum time is completed
                        //If it has then re-set the quantum
                        if ( quantum >= QUANTAM_TIME )
                        {
                                cur_process.log_working(dispatched_tick, cur_tick + 1);
                                ready_queue.push_back(ready_queue.front());
                                ready_queue.erase(ready_queue.begin());
                                if ( ready_queue.size() == 1 )
                                {
                                        dispatched_tick = cur_tick + 1;
                                }
                                quantum = 1;
                        }
			prev_process_id = cur_process_id;	// log the previous dispatched process ID
		}
                for ( int i = 0 ; i < intermediate.size() ; i++ )
                {
                        move_process_from(intermediate,ready_queue);
                }
		if(complete_num == processes.size()) {	// all process completed
			break;
		}
	}
	write_file(processes_done, output_path);	// write output
	return 1;
}

int fcfs(vector<process_t> processes, const char* output_path) 
{
	vector<process_t> ready_queue;
	vector<process_t> block_queue;
        vector<process_t> mutex_queue;
        vector<process_t> keyboard_queue;
        vector<process_t> intermediate;
	vector<process_t> processes_done;

	int complete_num = 0;
	int dispatched_tick = 0;
	int cur_process_id = -1, prev_process_id = -1;
        bool mutex_acquired = false;
        int quantum = 1;
	
	// main loop
	for(int cur_tick = 0; cur_tick < MAX_LOOP; cur_tick++) {
		// long term scheduler
		for(int i = 0; i < processes.size(); i++) {
			if(processes[i].arrival_time == cur_tick) {		// process arrives at current tick
				ready_queue.push_back(processes[i]);
			}
		}
                //We check is there is process mutex queue we add it to the ready queue and give it the lock
                if ( !mutex_queue.empty() )
                {
                        if ( mutex_lock(mutex_acquired) == true )
                        {
                                move_process_from(mutex_queue, ready_queue);
                        }
                        
                }
		// O device scheduling
		if(!block_queue.empty()) {
			process_t& cur_io_process = block_queue.front();	// always provide service to the first process in block queue
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// I/O service is completed
				cur_io_process.proceed_to_next_service();
				move_process_from(block_queue, intermediate);
			}
		}
                // I device scheduling
		if(!keyboard_queue.empty()) {
			process_t& cur_io_process = keyboard_queue.front();	// always provide service to the first process in block queue
			cur_io_process.cur_service_tick++;		// increment the num of ticks that have been spent on current service
			if(cur_io_process.cur_service_tick >= cur_io_process.cur_service.time_cost) {	// I/O service is completed
				cur_io_process.proceed_to_next_service();
				move_process_from(keyboard_queue, intermediate);
			}
		}
		// CPU scheduling
		if(ready_queue.empty()) {	// no process for scheduling
			prev_process_id = -1;	// reset the previous dispatched process ID to empty
		}
		else {
			ReadyQueueDealer(cur_process_id,prev_process_id,ready_queue,block_queue,keyboard_queue,processes_done,
                        mutex_queue,mutex_acquired,quantum,dispatched_tick,cur_tick,complete_num);

                        prev_process_id = cur_process_id;	// log the previous dispatched process ID
		}
                for ( int i = 0 ; i < intermediate.size() ; i++ )
                {
                        move_process_from(intermediate,ready_queue);
                }
		if(complete_num == processes.size()) {	// all process completed
			break;
		}
	}
	write_file(processes_done, output_path);	// write output
	return 1;
}

int main(int argc, char* argv[])
{
	if(argc != 4) {
		cout << "Incorrect inputs: too few arugments" << endl;
		return 0;
	}
	string scheduler = argv[1];
        const char* process_path = argv[2];
	const char* output_path = argv[3];
	vector<process_t> process_queue = read_processes(process_path);
        if ( scheduler == "FB" )
        {
                FB(process_queue, output_path);
        }
        else if ( scheduler == "RR" )
        {
                RR(process_queue, output_path);
        }
        else if ( scheduler == "FCFS" )
        {
                fcfs(process_queue, output_path);
        }
	return 0;
}
