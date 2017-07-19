#include <stdio.h>
#include <sqlite3.h>
#include <string.h>

sqlite3* db;
sqlite3_stmt *stmt;
sqlite3_stmt *st;

sqlite3* open_db(char* name){
	
	sqlite3* db;
	int rc;

	rc = sqlite3_open(name, &db);
	if(rc != SQLITE_OK){
		
		sqlite3_close(db);
		return NULL;
	}
	return db;
}

int create_tables(sqlite3* db, char* table){
	int rc;
	sqlite3_prepare_v2(db, table, strlen(table), &stmt, NULL);
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) return rc;
	else{
		sqlite3_finalize(stmt);
	}
	return rc;
}

int input_info(sqlite3* db, char* info){
	int rc;
	sqlite3_prepare_v2(db, info, strlen(info),&stmt, NULL);
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) return rc;
	else{
		sqlite3_finalize(stmt);
	}
	return rc;
}

int handle_selects(sqlite3* db, char* select){
	int rc;
	sqlite3_prepare_v2(db, select, -1, &st, NULL);
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_ROW) return rc;
	else{
		printf("%d\n", sqlite3_column_int(st,0));
		sqlite3_finalize(stmt);
	}
	return rc;
}  
