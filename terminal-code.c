#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <stdint.h>
#include "c-serial-calcs.h"
#include "db.h"
int init_tty(int fd);


/**
 * Run this program and pass the location of the serial file descriptor as an argument
 * On linux, this is likely /dev/ttyACM0
 * On OSX, this may be similar to /dev/cu.usbmodem1D1131
 */

int main(int argc, char **argv) {

    /*
     * Read the device path from input,
     * or default to /dev/ttyACM0
     */

    if (argc == 2) {
        device = argv[1];
    } else {
        device = "/dev/tty.usbmodem1411";
    }
    printf("Connecting to %s\n", device);

    /*
     * Need the following flags to open:
     * O_RDWR: to read from/write to the devices
     * O_NOCTTY: Do not become the process's controlling terminal
     * O_NDELAY: Open the resource in nonblocking mode
     */

    printf("fd\n");
    fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        perror("Error opening serial");
        return -1;
    }
    printf("config\n");
    /* Configure settings on the serial port */
    if (init_tty(fd) == -1) {
        perror("init");
        close(fd);
        return -1;
    }

    printf("flush\n");
    /* Flush whatever is remaining in the buffer */
    tcflush(fd, TCIFLUSH);

    get_date();

    /*int pid = fork();
    if (pid < 0){ // error
        perror("Error forking process...quitting!\n");
	return -1;
    }

    if (pid == 0){
	printf("Child\n");
	child_work();
	//Prints when child process has exited
	printf("Aborting child\n");
    }else{
	printf("Parent\n");
	child_pid = pid;
	parent_work();
	//Prints when parent process has exited
	printf("Leaving parent\n");
    }*/

    printf("Program is done!\n");
    parent_work();

    close(hist_fd);
    close(fd);
    return -1;
}

int mmap_file(){

    const char *text = "0";

    /* Open a file for writing.
     *  - Creating the file if it doesn't exist.
     *  - Truncating it to 0 size if it already exists. (not really needed)
     *
     * Note: "O_WRONLY" mode is not sufficient when mmaping.
     */

    const char *filepath = "histogramiscool";

    hist_fd = open(filepath, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);

    if (hist_fd == -1){
        perror("Error opening file for writing");
        exit(EXIT_FAILURE);
    }

    //Stretch the file size to the size of the (mmapped) array of char

    size_t textsize = strlen(text) + 1; // + \0 null character

    if (lseek(hist_fd, textsize-1, SEEK_SET) == -1){
        close(hist_fd);
        perror("Error calling lseek() to 'stretch' the file");
        exit(EXIT_FAILURE);
    }


    if (write(hist_fd, "", 1) == -1){
        close(hist_fd);
        perror("Error writing last byte of the file");
        exit(EXIT_FAILURE);
    }


    // Now the file is ready to be mmapped.
    int* histo;
    histo = mmap(0, textsize, PROT_READ | PROT_WRITE, MAP_SHARED, hist_fd, 0);

    if (histo == MAP_FAILED){
        close(hist_fd);
        perror("Error mmapping the file");
        exit(EXIT_FAILURE);
    }

    // Write it now to disk
    if (msync(histo, textsize, MS_SYNC) == -1)
    {
        perror("Could not sync the file to disk");
    }

    return 0;
}

int parent_read_ar(){
	cstruct = malloc(sizeof(struct data_coll));
	cstruct->_BPM = 0;
	cstruct->_hr = 0;
	cstruct->_min = 0;
	cstruct->_temp = 0;
	int found_end = 0; //if ending byte if read
	unsigned char beginbyte = 0x7e;
	unsigned char endbyte = 0x7f;
	char check = 0x00; //used for reading values from Arduino
	int index = 0;
	int value = 0;
	char begin = 'z'; //tells Arduino to send info
	data_array[0] = 0;
	data_array[1] = 0;
	data_array[2] = 0;
	data_array[3] = 0;
	value = write(fd, &begin, 1);
	if (value == -1){
	    perror("write");
       	    close(fd);
	    return -1;
	}else if (value == 0) {
	    fprintf(stderr, "No data written\n");
	    close(fd);
	    return -1;
	}
	
	value = read(fd, &check, 1);
	if(value == -1){
		perror("read");
		close(fd);
		return -1;
	}else if(value == 0){
		fprintf(stderr, "No data returned\n");
		close(fd);
		return -1;
	}

	if(check == beginbyte){
		check = 0x00;
		//printf("writing to arduino...");
		//Tells Arduino to start sending info
		value = write(fd, &begin, 1);
		printf("(%d) %s\n", value, &begin);
		if (count == -1) {
		    //perror("write");
		    close(fd);
		    return -1;
		} else if (count == 0) {
		    //fprintf(stderr, "No data written\n");
		    close(fd);
		    return -1;
		}
			sleep(1);
		do{
			//printf("reading from arduino...");
			value = read(fd, &check, 1);
			//printf("(%d) %s\n", value, &check);
			if(check == endbyte){
				found_end = 1;
			}else{
				data_array[index] += check;
				index++;
				check = 0x00;
			}
		}while(!found_end);
		cstruct->_BPM = data_array[0];
		cstruct->_hr = data_array[1];
		cstruct->_min = data_array[2];
		cstruct->_temp = data_array[3];
		add_to_gram();
		data_array[0] = 0;
		data_array[1] = 0;
		data_array[2] = 0;
		data_array[3] = 0;
		index = 0;
		value = 0;
	}else{ printf("not beginning: %s\n", &check); }
	return 0;
}

//Takes state of RTC from Arduino
int get_date(){
	char incoming;
	char dbyte = 'd';
	int i = 0;
	int endthis = 0;
	unsigned char bbyte = 0x7e;
	unsigned char ebyte = 0x7f;
	time_array[0] = 0;
	time_array[1] = 0;
	time_array[2] = 0;
	count = write(fd, &dbyte, 1);
	sleep(1);
	count = read(fd, &incoming, 1);
	if(incoming == bbyte){
		while(!endthis){
			count = read(fd, &incoming, 1);
			if(incoming == ebyte){
				endthis = 1;
			}else{
				time_array[i] += incoming;
				i++;
			}
		}
		month = time_array[0];
		day = time_array[1];
		year = time_array[2];
	}else{
		printf("Could not set date! Try again...\n");
		return -1;
	}
	return 0;
}

/*int callback(void *arg, int argc, char** argv, char** colName){
	int i;
	for(i=0;i<argc;i++){
		printf("%s = %s\t", colName[i], argv[i] ? : "NULL");
	}
	printf("\n");
	return 0;
}*/

int child_work(){
	c2a = malloc(sizeof(struct command_ar*));
	while(c2a->command != 'e'){
		char* query;
		int rc,m;
		printf("Enter command: \n");
		printf("d:date\nn:environment\nx:hist X\np:pause\na:rate\ng:regression X\nc:reset\nr:resume\nt:stat X\ns:show X\n");
		c2a->command = getchar();
		puts(&c2a->command);

		//Pause for given time then resumes
		if(c2a->command == 'p'){
			count = write(fd, &c2a->command, 1);
			//sleep(1000*3000);
		//prints given value to display and console
		}else if(c2a->command == 's'){
			char x;
			x = getchar();	
			count = write(fd, &c2a->command, 1);
			sleep(1);
			count = write(fd, &x, 1);
			//sleep(1000);
			printf("Desired bpm value: ");
			puts(&x);
		//resumes default data collection
		}else if(c2a->command == 'r'){
			count = write(fd, &c2a->command, 1);
		//Prints current sensor value
		}else if(c2a->command  == 'n'){
			printf("Current temprature: %d\n", c_temp);
		//Prints current heart rate value
		}else if(c2a->command == 'a'){
			printf("Last recorded heart rate: %d\n", c_BPM);
		//Prints state of RTC
		}else if(c2a->command == 'd'){
			printf("%d %d,%d %d:%d\n", day, month, year, c_hr, c_min);
		// Prints our linear regression formula
		}else if(c2a->command == 'g'){
			asprintf(&query, "SELECT ((SUM(B.bpm)*SUM(T.temps*T.temps)-SUM(T.temps)*SUM(T.temps*B.bpm))/(COUNT(B.bpm)*SUM(T.temps*T.temps))-(SUM(T.temps)*SUM(T.temps))) FROM Beats B, Temps T WHERE hour = %d AND minute >= %d;",cstruct->_hr,cstruct->_min);
			printf("Linear Regression: Y = ");
			rc = handle_selects(db, query);
			if(rc != SQLITE_ROW) printf("Trouble executing regression statement\n");
			
			printf("X + ");
			asprintf(&query, "SELECT ((Count(B.bpm)*SUM(B.bpm*T.temps)-SUM(T.temps)*SUM(B.bpm))/(COUNT(B.bpm)*SUM(T.temps*T.temps))-(SUM(T.temps)*SUM(T.temps))) FROM Beats B, Temps T WHERE hour = %d AND minute >= %d;",cstruct->_hr,cstruct->_min);
			rc = handle_selects(db, query);
			if(rc != SQLITE_ROW) printf("Trouble executing regression statement\n");
			
			free(query);
		//Prints desired statistic of both sensor and hr values
		}else if(c2a->command == 't'){
			int stat;
			printf("Choose a number for a statistic...\n1: reading count\n2: mean\n3: median\n4: mode\n5: std dev\n");
			scanf("%d\n", &stat);
			if(stat == 1){ // count
				asprintf(&query, "SELECT count(*) AS num_beats FROM Beats WHERE hour = %d AND minute >= %d;",cstruct->_hr,cstruct->_min);
				printf("Beats count: ");
				rc = handle_selects(db, query);
				if(rc != SQLITE_ROW) printf("Trouble executing count statement\n");

				asprintf(&query, "SELECT count(*) AS num_temps FROM Temp WHERE hour = %d AND minute >= %d;",cstruct->_hr,cstruct->_min);
				printf("Temperature count: ");
				rc = handle_selects(db, query);
				if(rc != SQLITE_ROW) printf("Trouble executing count statement\n");
				free(query);
			}else if(stat == 2){ // mean
				asprintf(&query, "SELECT AVG(*) AS mean_beats FROM Beats WHERE hour = %d AND minute >= %d;",cstruct->_hr,cstruct->_min);
				printf("Beats mean: ");
				rc = handle_selects(db, query);
				if(rc != SQLITE_ROW) printf("Trouble executing mean statement\n");

				asprintf(&query, "SELECT AVG(*) AS mean_temps FROM Temp WHERE hour = %d AND minute >= %d;",cstruct->_hr,cstruct->_min);
				printf("Temperature mean: ");
				rc = handle_selects(db, query);
				if(rc != SQLITE_ROW) printf("Trouble executing mean statement\n");
				free(query);
			}else if(stat == 3){ // median
				asprintf(&query, "SELECT bpm FROM Beats WHERE hour = %d AND minute >= %d ORDER BY ASC Limit 1 OFFSET (SELECT COUNT(*) FROM Beats) /2;",cstruct->_hr,cstruct->_min);
				printf("Median of Beats: ");
				rc = handle_selects(db, query);
				if(rc != SQLITE_ROW) printf("Trouble executing median statement\n");

				asprintf(&query, "SELECT temps FROM Temp WHERE hour = %d AND minute >= %d ORDER BY ASC Limit 1 OFFEST (SELECT COUNT(*) FROM Temps) /2;",cstruct->_hr,cstruct->_min);
				printf("Median of Temperature: ");
				rc = handle_selects(db, query);
				if(rc != SQLITE_ROW) printf("Trouble executing median statement\n");
				free(query);
			}else if(stat == 4){ // mode
				asprintf(&query, "SELECT bpm FROM Beats WHERE hour = %d AND minute >= %d GROUP BY bpm ORDER BY COUNT(*) DESC Limit 1;",cstruct->_hr,cstruct->_min);

				printf("Beats Mode: ");
				rc = handle_selects(db, query);
				if(rc != SQLITE_ROW) printf("Trouble executing mode statement\n");

				asprintf(&query, "SELECT temps FROM Temp WHERE hour = %d AND minute >= %d GROUP BY bpm ORDER BY COUNT(*) DESC Limit 1;",cstruct->_hr,cstruct->_min);
				printf("Temperature Mode: ");
				rc = handle_selects(db, query);

				if(rc != SQLITE_ROW) printf("Trouble executing mode statement\n");
				free(query);
			}else if(stat == 5){ // standard deviation
				printf("Beats Standard Deviation\n");
				printf("-----------");
				printf("The Square root of ");
				asprintf(&query, "SELECT SUM(bpm-AVG(bpm))/COUNT(bpm) as stdev FROM Beats WHERE hour = %d AND minute >= %d;",cstruct->_hr,cstruct->_min);
				rc = handle_selects(db, query);
				if(rc != SQLITE_ROW) printf("Trouble executing std dev statement\n");

				printf("Temperature std dev\n");
				printf("-----------");
				printf("The Square root of ");
				asprintf(&query, "SELECT SUM(temps-AVG(temps))/COUNT(temps) as stdev FROM Temp WHERE hour = %d AND minute >= %d;",cstruct->_hr,cstruct->_min);
				rc = handle_selects(db,query);

				if(rc != SQLITE_ROW) printf("Trouble executing std dev statement\n");
				free(query);
			}
		}
		c2a->command = 0x00;
	}//end of while loop
	return 0;
}

int parent_work(){
	int rc, status;
	char* query;
	//create database or access existing db
	db = open_db("temp_bpm.db");//rc = sqlite3_open("temp_bpm.db", &db);
	if(db == NULL){
	    perror("Error opening database\n");
	    _exit(-1);
	}

	//create tables
	asprintf(&query, "CREATE TABLE IF NOT EXISTS Beats(bpm INTEGER NOT NULL, minute INTEGER NOT NULL, hour INTEGER NOT NULL);");
	rc = create_tables(db, query);
	if(rc != SQLITE_DONE){
	    perror("Error creating beats table\n");
	    _exit(-1);
	}

	asprintf(&query, "CREATE TABLE IF NOT EXISTS Temp(degrees INTEGER NOT NULL, minute INTEGER NOT NULL, hour INTEGER NOT NULL);");

	rc = create_tables(db, query);
	if(rc != SQLITE_DONE){
	    perror("Error creating temp tables\n");
	    _exit(-1);
	}

	//create histogram file
	mmap_file();


	sleep(1000*500); //.5 seconds

	//Continues until child quits
	cstruct = malloc(sizeof(struct data_coll*));
	while(1){//waitpid(child_pid, &status, WNOHANG) == 0){
	    parent_read_ar();
	    set_vals();
	    query_beats();
	    query_temp();
	}
	return 0;
}

int set_vals(){
	c_BPM = cstruct->_BPM;
	c_hr = cstruct->_hr;
	c_temp = cstruct->_temp;
	c_min = cstruct->_min;
	return 0;
}

int  add_to_gram(){
	int x_time = (cstruct->_hr*4)+(cstruct->_min%15);
	int y_bts = cstruct->_BPM - 40;
	//add to histogram mmaped file
	**histogram[x_time][y_bts] = cstruct->_BPM;
	return 0;
}

int query_temp(){
    int rc;
    char *query = NULL;
    asprintf(&query, "insert into Temp(degrees, minute, hour) values (%d,%d,%d);",cstruct->_temp,cstruct->_min,cstruct->_hr);
    
    rc = input_info(db, query); // step executes the statement
    if (rc != SQLITE_DONE) { // Error
        printf("ERROR inserting temperature data");
        return -1;
    }
    free(query);
    return 0;
}

int query_beats(){
    int rc;
    char* query = NULL;
    asprintf(&query, "insert into Beats(bpm, minute, hour) values(%d,%d,%d);",cstruct->_BPM,cstruct->_min,cstruct->_hr);
    
    rc = input_info(db, query); // step executes the statement
    if (rc != SQLITE_DONE) { // Error
        printf("ERROR inserting beats data");
        return -1;
    }
    free(query);
    return 0;
}

int init_tty(int fd) {
    struct termios tty;
    /*
     * Configure the serial port.
     * First, get a reference to options for the tty
     * Then, set the baud rate to 9600 (same as on Arduino)
     */
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) == -1) {
        perror("tcgetattr");
        return -1;
    }

    if (cfsetospeed(&tty, (speed_t)B9600) == -1) {
        perror("ctsetospeed");
        return -1;
    }
    if (cfsetispeed(&tty, (speed_t)B9600) == -1) {
        perror("ctsetispeed");
        return -1;
    }

    // 8 bits, no parity, no stop bits
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // No flow control
    tty.c_cflag &= ~CRTSCTS;

    // Set local mode and enable the receiver
    tty.c_cflag |= (CLOCAL | CREAD);

    // Disable software flow control
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Make raw
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    // Infinite timeout and return from read() with >1 byte available
    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    // Update options for the port
    if (tcsetattr(fd, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        return -1;
    }

    return 0;
}
