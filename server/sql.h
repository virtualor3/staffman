#ifndef __SQL_H__
#define __SQL_H__

#include <sqlite3.h>

#include "protocol.h"

struct sqlhandle {
    sqlite3_stmt* pStmt;
};

static const char* memberlst[] = { "id", "usertype", "name", "password", "contact", "address", "office", "date", "age", "pay", "level" };

void sql_init(void);
void sql_close(void);

int User_exist(uint32_t id);
struct userinfo* User_search_all(struct sqlhandle* hd, struct userinfo* info);
struct userinfo* User_search_all_with_id(struct sqlhandle* hd, uint32_t id, struct userinfo* info);
extern struct userinfo* User_search_all_with_name(struct sqlhandle* hd, const char* name, struct userinfo* info);

extern struct userinfo* User_search_loginfo_with_id(struct sqlhandle* hd, uint32_t id, struct userinfo* info);
extern struct userinfo* User_search_loginfo_with_name(struct sqlhandle* hd, const char* name, struct userinfo* info);

int User_insert(struct userinfo* info);
int User_update(uint32_t id, enum entry key, const char* value);
int User_delete(uint32_t id);

struct historylog* history_search(struct sqlhandle* hd, struct historylog* log);
int history_insert(struct historylog* log);

#endif