#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <stdint.h>
#include <sqlite3.h>

typedef int boolean;
enum{false,true};

struct data_coll{
    int _BPM;
    int _hr;
    int _min;
    int _temp;
};

struct command_ar{
    char command;
};


struct data_coll *cstruct;
struct command_ar* c2a;
sqlite3* db;
int **histogram[96][61];
int fd, hist_fd, share_fd, child_pid;
int value, count, hour, minute, BPM = 0;
char *device;
char byte;
char buff[256]; 
pid_t child_pid;
int currentIndex;
int c_BPM, c_hr, c_temp, c_min, month, day, year;
int data_array[4], time_array[3];

//struct curr_state *create();

int add_to_gram();

int child_read_ar();

int parent_read_ar();

//void print_gram(int hr, int min);

//void print_current_time(int hr, int min);

int child_work();

int parent_work();

int mmapp_file();

int query_beats();

int query_temp();

/*Takes string input from Arduino with RTC state
 *and stores values in to global variables
 */
int get_date();

/*Used when executing sql selection statements*/
int callback(void *arg, int argc, char** argv, char** colName);

/*Sets the values in the global cstruct
 *to integer variables that allow for 
 *more secure access when commanded
 */
int set_vals();
