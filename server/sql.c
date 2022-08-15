#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>

#include "staffman.h"
#include "sql.h"

#define SQL_SIZE_MAX	256
#define CREATE_USERINFO	"create table if not exists userinfo(id integer primary key autoincrement,"	\
						"usertype int,"	\
						"name char,"	\
						"password char,"	\
						"contact char,"	\
						"address char,"	\
						"office char,"	\
						"date char,"	\
						"age int,"	\
						"pay char,"	\
						"level int)"

#define CREATE_HISTORY  "create table if not exists history(time int, id int, event char)"
#define INSERT_USERINFO	"insert into userinfo values(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11)"
#define UPDATE_USERINFO	"update userinfo set %s=? where id=?"
#define DELETE_USERINFO	"delete from userinfo where id=?"

#define sql_strcpy_safe(dest, icol)	({if (sqlite3_column_type(hd->pStmt, icol) == SQLITE_TEXT)	\
		strcpy(dest, sqlite3_column_text(hd->pStmt, icol));	\
	else strcpy(dest, "NULL");})	\


static sqlite3* userdb = NULL;	//用户信息和历史记录数据库句柄

void sql_init(void)
{
	if (sqlite3_open("./.usr.db", &userdb) != SQLITE_OK) {
		printf("%s: %d %s\n", __func__, sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		exit(EXIT_FAILURE);
	}
	log("database open success!");
	sqlite3_exec(userdb, "pragma synchronous = OFF", NULL, NULL, NULL);
	//创建用户信息表userinfo
	sqlite3_exec(userdb, CREATE_USERINFO, NULL, NULL, NULL);
	log("table usrinfo already exists.\n");
	//创建历史记录表history
	sqlite3_exec(userdb, CREATE_HISTORY, NULL, NULL, NULL);
	log("table history already exists.\n");
}

void sql_close(void)
{
	sqlite3_close(userdb);
}

/**
 * @brief User_exist - 判断表userinfo中id对应的记录是否存在
 * @param id 识别id
 * @return int - 0表示记录不存在；1表示记录存在；-1表示发生错误
 */
int User_exist(uint32_t id)
{
	int ret;
	sqlite3_stmt* pStmt;
	if (sqlite3_prepare_v2(userdb, "select id from userinfo where id=?", -1, &pStmt, NULL) != SQLITE_OK) {
		Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		sqlite3_finalize(pStmt);
		return -1;
	}
	sqlite3_bind_int(pStmt, 1, id);
	ret = sqlite3_step(pStmt);
	if (ret == SQLITE_ROW) {		//表项存在
		sqlite3_finalize(pStmt);
		return 1;
	}
	if (ret == SQLITE_DONE) {		//表项不存在
		sqlite3_finalize(pStmt);
		return 0;
	}
	sqlite3_finalize(pStmt);
	Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
	return -1;
}

/**
 * _sql_search_all - 查找用户所有信息
 * @param hd: sqlhandle - sql相关参数句柄，保存上下文信息
 * @param sql: sqlite3数据库sql执行语句
 * @param info: 用于接收从数据库查找到的每条记录，基地址作为返回值返回
 * @return struct userinfo* - 返回指向保存每次查找到的记录数据的指针。
 * 返回值非NULL表示找到一条数据;返回NULL表示查找结束,没有找到记录
 */
static struct userinfo* _sql_search_all(struct sqlhandle* hd, const char* sql, struct userinfo* info)
{
	int ret = 0;
	if (hd->pStmt == NULL) {
		if (sqlite3_prepare_v2(userdb, sql, -1, &hd->pStmt, NULL) != SQLITE_OK) {
			Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
			sqlite3_finalize(hd->pStmt);
			return NULL;
		}
	}
	if ((ret = sqlite3_step(hd->pStmt)) == SQLITE_ROW) {
		info->id = sqlite3_column_int(hd->pStmt, 0);
		info->usertype = sqlite3_column_int(hd->pStmt, 1);
		sql_strcpy_safe(info->name, 2);
		sql_strcpy_safe(info->password, 3);
		sql_strcpy_safe(info->contact, 4);
		sql_strcpy_safe(info->address, 5);
		sql_strcpy_safe(info->office, 6);
		sql_strcpy_safe(info->date, 7);
		info->age = sqlite3_column_int(hd->pStmt, 8);
		info->pay = sqlite3_column_int(hd->pStmt, 9);
		info->level = sqlite3_column_int(hd->pStmt, 10);
		return info;
	}
	if (ret == SQLITE_DONE) {
		sqlite3_finalize(hd->pStmt);
		hd->pStmt = NULL;
		return NULL;
	}
	sqlite3_finalize(hd->pStmt);
	Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
	return NULL;
}

/**
 * sql_search_loginfo - 查找用户的登录相关信息: id, name, password, usertype
 * @param hd: sqlhandle - sql相关参数句柄，保存上下文信息
 * @param sql: sqlite3数据库sql执行语句
 * @param info: 用于接收从数据库查找到的每条记录，基地址作为返回值返回
 * @return struct userinfo* - 返回指向保存每次查找到的记录数据的指针。
 * 返回值非NULL表示找到一条数据;返回NULL表示查找结束,没有找到记录
*/
static struct userinfo* _sql_search_loginfo(struct sqlhandle* hd, const char* sql, struct userinfo* info)
{
	int ret;
	if (hd->pStmt == NULL) {
		if (sqlite3_prepare_v2(userdb, sql, -1, &hd->pStmt, NULL) != SQLITE_OK) {
			Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
			sqlite3_finalize(hd->pStmt);
			return NULL;
		}
	}
	if ((ret = sqlite3_step(hd->pStmt)) == SQLITE_ROW) {
		info->id = sqlite3_column_int(hd->pStmt, 0);
		info->usertype = sqlite3_column_int(hd->pStmt, 1);
		sql_strcpy_safe(info->password, 2);
		return info;
	}
	if (ret == SQLITE_DONE) {
		sqlite3_finalize(hd->pStmt);
		hd->pStmt = NULL;
		return NULL;
	}
	Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
	return NULL;
}

/**
 * user_search_all - 查找用户的所有信息
 * @param hd: sqlhandle - sql相关参数句柄，保存上下文信息
 * @param info: 用于接收从数据库查找到的每条记录，基地址作为返回值返回
 * @return struct userinfo* - 返回指向保存每次查找到的记录数据的指针。
 * 返回值非NULL表示找到一条数据;返回NULL表示查找结束,没有找到记录
*/
struct userinfo* User_search_all(struct sqlhandle* hd, struct userinfo* info)
{
	return _sql_search_all(hd, "select * from userinfo", info);
}

/**
 * user_search_all_with_id - 按照id查找所有用户信息
 * @param hd: sqlhandle - sql相关参数句柄，保存上下文信息
 * @param id: 索引
 * @param info: 用于接收从数据库查找到的每条记录，基地址作为返回值返回
 * @return struct userinfo* - 返回指向保存每次查找到的记录数据的指针。
 * 返回值非NULL表示找到一条数据;返回NULL表示查找结束,没有找到记录
*/
struct userinfo* User_search_all_with_id(struct sqlhandle* hd, uint32_t id, struct userinfo* info)
{
	char sql[64];
	if (hd->pStmt == NULL) sprintf(sql, "select * from userinfo where id=%d", id);
	return _sql_search_all(hd, sql, info);
}

/**
 * user_search_all_with_name - 按照name查找所有用户信息
 * @param hd: sqlhandle - sql相关参数句柄，保存上下文信息
 * @param name: 索引
 * @param info: 用于接收从数据库查找到的每条记录，基地址作为返回值返回
 * @return struct userinfo* - 返回指向保存每次查找到的记录数据的指针。
 * 返回值非NULL表示找到一条数据;返回NULL表示查找结束,没有找到记录
*/
struct userinfo* User_search_all_with_name(struct sqlhandle* hd, const char* name, struct userinfo* info)
{
	char sql[64];
	if (hd->pStmt == NULL) sprintf(sql, "select * from userinfo where name=\"%s\"", name);
	return _sql_search_all(hd, sql, info);
}

/**
 * User_search_loginfo_with_id - 按照id查找用户的登录相关信息: id, name, password, usertype
 * @param hd: sqlhandle - sql相关参数句柄，保存上下文信息
 * @param name: 索引
 * @param info: 用于接收从数据库查找到的每条记录，基地址作为返回值返回
 * @return struct userinfo* - 返回指向保存每次查找到的记录数据的指针。
 * 返回值非NULL表示找到一条数据;返回NULL表示查找结束,没有找到记录
*/
struct userinfo* User_search_loginfo_with_id(struct sqlhandle* hd, uint32_t id, struct userinfo* info)
{
	char sql[128];
	if (hd->pStmt == NULL) {
		sprintf(sql, "select id,usertype,password from userinfo where id=%d", id);
	}
	return _sql_search_loginfo(hd, sql, info);
}

/**
 * User_search_loginfo_with_name - 按照name查找用户的登录相关信息: id, name, password, usertype
 * @param hd: sqlhandle - sql相关参数句柄，保存上下文信息
 * @param name: 索引
 * @param info: 用于接收从数据库查找到的每条记录，基地址作为返回值返回
 * @return struct userinfo* - 返回指向保存每次查找到的记录数据的指针。
 * 返回值非NULL表示找到一条数据;返回NULL表示查找结束,没有找到记录
*/
struct userinfo* User_search_loginfo_with_name(struct sqlhandle* hd, const char* name, struct userinfo* info)
{
	char sql[128];
	if (hd->pStmt == NULL) sprintf(sql, "select id,usertype,password from userinfo where name=\"%s\"", name);
	return _sql_search_loginfo(hd, sql, info);
}

/**
 * User_insert - 插入新用户信息
 * @param info 保存用户信息
 * @return int 插入成功返回0， 失败返回-1
 */
int User_insert(struct userinfo* info)
{
	sqlite3_stmt* pStmt = NULL;
	if (sqlite3_prepare_v3(userdb, INSERT_USERINFO, -1, 0, &pStmt, NULL) != SQLITE_OK) {
		Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		sqlite3_finalize(pStmt);
		return -1;
	}
	sqlite3_bind_int(pStmt, 1, info->id);
	sqlite3_bind_int(pStmt, 2, info->usertype);
	sqlite3_bind_text(pStmt, 3, info->name, -1, SQLITE_STATIC);
	sqlite3_bind_text(pStmt, 4, info->password, -1, SQLITE_STATIC);
	sqlite3_bind_text(pStmt, 5, info->contact, -1, SQLITE_STATIC);
	sqlite3_bind_text(pStmt, 6, info->address, -1, SQLITE_STATIC);
	sqlite3_bind_text(pStmt, 7, info->office, -1, SQLITE_STATIC);
	sqlite3_bind_text(pStmt, 8, info->date, -1, SQLITE_STATIC);
	sqlite3_bind_int(pStmt, 9, info->age);
	sqlite3_bind_int(pStmt, 10, info->pay);
	sqlite3_bind_int(pStmt, 11, info->level);
	if (sqlite3_step(pStmt) != SQLITE_DONE) {
		Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		sqlite3_finalize(pStmt);
		return -1;
	}
	sqlite3_finalize(pStmt);
	return 0;
}

int User_update(uint32_t id, enum entry key, const char* value)
{
	char sql[64];
	sqlite3_stmt* pStmt = NULL;
	sprintf(sql, UPDATE_USERINFO, memberlst[key - STAFF_BASE]);
	if (sqlite3_prepare_v2(userdb, sql, -1, &pStmt, NULL) != SQLITE_OK) {
		Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		sqlite3_finalize(pStmt);
		return -1;
	}
	sqlite3_bind_text(pStmt, 1, value, -1, SQLITE_STATIC);
	sqlite3_bind_int(pStmt, 2, id);
	if (sqlite3_step(pStmt) != SQLITE_DONE) {
		Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		sqlite3_finalize(pStmt);
		return -1;
	}
	sqlite3_finalize(pStmt);
	return 0;
}

int User_delete(uint32_t id)
{
	sqlite3_stmt* pStmt = NULL;
	if (sqlite3_prepare_v2(userdb, DELETE_USERINFO, -1, &pStmt, NULL) != SQLITE_OK) {
		Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		sqlite3_finalize(pStmt);
		return -1;
	}
	sqlite3_bind_int64(pStmt, 1, (sqlite3_int64)id);
	if (sqlite3_step(pStmt) != SQLITE_DONE) {
		Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		sqlite3_finalize(pStmt);
		return -1;
	}
	sqlite3_finalize(pStmt);
	return 0;
}

//查找所有历史记录
struct historylog* history_search(struct sqlhandle* hd, struct historylog* log)
{
	int ret;
	if (hd->pStmt == NULL) {
		if (sqlite3_prepare_v2(userdb, "select * from history", -1, &hd->pStmt, NULL) != SQLITE_OK) {
			Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
			sqlite3_finalize(hd->pStmt);
		}
	}
	if ((ret = sqlite3_step(hd->pStmt)) == SQLITE_ROW) {
		log->time = sqlite3_column_int64(hd->pStmt, 0);
		log->id = sqlite3_column_int(hd->pStmt, 1);
		strcpy(log->event, sqlite3_column_text(hd->pStmt, 2));
		return log;
	}
	if (ret == SQLITE_DONE) {
		sqlite3_finalize(hd->pStmt);
		hd->pStmt = NULL;
		return NULL;
	}
	Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
	return NULL;
}

//插入历史记录
int history_insert(struct historylog* log)
{
	sqlite3_stmt* pStmt = NULL;
	if (sqlite3_prepare_v2(userdb, "insert into history values(?, ?, ?)", -1, &pStmt, NULL) != SQLITE_OK) {
		Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		sqlite3_finalize(pStmt);
		return -1;
	}
	sqlite3_bind_int64(pStmt, 1, log->time);
	sqlite3_bind_int(pStmt, 2, log->id);
	sqlite3_bind_text(pStmt, 3, log->event, -1, SQLITE_STATIC);
	if (sqlite3_step(pStmt) != SQLITE_DONE) {
		Dbug("[%d: %s]\n", sqlite3_errcode(userdb), sqlite3_errmsg(userdb));
		sqlite3_finalize(pStmt);
		return -1;
	}
	sqlite3_finalize(pStmt);
	return 0;
}