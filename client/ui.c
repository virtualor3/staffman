#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

#include "staffman.h"
#include "ui.h"

//登录菜单
#define MENU_LOGIN  \
"\ec****************************************************************\n" \
"***************1:管理员模式  2:普通用户模式  3:退出*************\n"  \
"****************************************************************\n"
//管理员菜单
#define MENU_ADMINOPER   "\ec亲爱的管理员,欢迎您登陆员工管理系统!\n"    \
"********************************************************************\n" \
"** 1:查询  2:修改  3:添加用户  4:删除用户  5:查询历史记录  6:退出 **\n"  \
"********************************************************************\n"
//普通用户菜单
#define MENU_NORMALOPER   "\ec亲爱的用户,欢迎您登陆员工管理系统!\n" \
"****************************************************************\n"    \
"********************1:查询    2:修改    3:退出******************\n"   \
"****************************************************************\n"
//管理员查找菜单
#define MENU_ADMINQUERY   \
"\ec****************************************************************\n"   \
"**************** 1:按人名查找  2:查找所有  3:退出 **************\n"  \
"****************************************************************\n"
//数据修改菜单
#define MENU_ADMINCHANGE \
"\ec***********************请输入要修改的选项************************\n"  \
"****** 1:姓名    2:年龄     3:家庭住址   4:电话    5:职位  ******\n"    \
"****** 6:工资    7:入职日期 8:评级       9:密码    10:退出 ******\n"    \
"*****************************************************************\n"
//数据修改菜单
#define MENU_NORMALCHANGE \
"\ec***********************请输入要修改的选项************************\n"  \
"************ 1:家庭住址    2:电话    3:密码    4:退出 ***********\n"    \
"*****************************************************************\n"

//时间格式转换
char* getLocalTime(time_t time, char* strbuf)
{
    struct tm* tim = localtime(&time);
    sprintf(strbuf, "%4d-%02d-%02d %02d:%02d:%02d", tim->tm_year + 1900,
        tim->tm_mon + 1, tim->tm_mday, tim->tm_hour, tim->tm_min, tim->tm_sec);
    return strbuf;
}

const char* deal_errcode(struct msg_ack* msg)
{
    const char* ret;
    switch (msg->err) {
        case ERR_PASSWD:    ret = "密码错误"; break;
        case ERR_NONUSER:   ret = "用户不存在"; break;
        case ERR_NOADMIN:   ret = "非管理员用户"; break;
        default:            ret = "";
    }
    return ret;
}

void do_server_exit(int fd)
{
    close(fd);
    printf("服务器离线,程序退出!...");
    exit(EXIT_SUCCESS);
}

void do_login(int fd, struct userhandle* userhd)
{
    int num;
    struct msg_login* msg = (struct msg_login*)userhd->msg;

    printf(MENU_LOGIN);         //打印登录菜单
    while (True) {
        printf("input(1~3)>>"); fflush(stdout);
        scanf("%d", &num);
        while (getchar() != '\n');
        while (num == 0 || num > 3) {
            printf("输入错误,请重新输入!\ninput(1~3)>>"); fflush(stdout);
            scanf("%d", &num);
        }
        if (num == 3) {
            close(fd);
            printf("退出成功!\n");
            exit(EXIT_SUCCESS);
        }
        printf("请输入工号: "); fflush(stdout);
        scanf("%d", &userhd->id);
        while (getchar() != '\n');

        printf("请输入密码: "); fflush(stdout);
        scanf("%s", msg->password);
        while (getchar() != '\n');

        msg->msg.cmdcode = CLI_LOGIN;
        msg->msg.size = sizeof(struct msg_login);
        msg->id = userhd->id;

        if (num == 1) msg->usertype = ADMINISTRATOR;
        else if (num == 2) msg->usertype = NORMAL;

        userhd->usertype = msg->usertype;

        CHEAK(send(fd, (void*)msg, msg->msg.size, 0), -1);
        if (recv_msg(fd, (struct msg*)msg) == 0) do_server_exit(fd);

        if (msg->msg.cmdcode == SER_SUCCESS) {
            if (num == 1) do_admin_oper(fd, userhd);
            else if (num == 2) do_normal_oper(fd, userhd);
            printf(MENU_LOGIN);         //打印登录菜单
        } else if (msg->msg.cmdcode == SER_ERR) {
            printf("%s\n", deal_errcode((struct msg_ack*)msg));
        }
    }
}

void do_logout(int fd, struct userhandle* userhd)
{
    struct msg_logout* msg = (struct msg_logout*)userhd->msg;
    msg->id = userhd->id;
    msg->msg.cmdcode = CLI_LOGOUT;
    msg->msg.size = sizeof(struct msg_logout);
    send(fd, (void*)msg, msg->msg.size, 0);
}

void do_admin_oper(int fd, struct userhandle* userhd)
{
    uint32_t num;
    printf(MENU_ADMINOPER);         //打印管理员菜单
    while (True) {
        printf("input>");
        fflush(stdout);
        scanf("%d", &num);

        switch (num) {
            case 1: do_admin_query(fd, userhd); goto print_menu;
            case 2: do_admin_change(fd, userhd); goto print_menu;
            case 3: do_admin_adduser(fd, userhd); goto print_menu;
            case 4: do_admin_deluser(fd, userhd); break;
            case 5: do_admin_history(fd, userhd); break;
            case 6: do_logout(fd, userhd); return;
        }
        continue;
    print_menu:
        printf(MENU_ADMINOPER);         //打印管理员菜单
    }
}

void do_normal_oper(int fd, struct userhandle* userhd)
{
    uint32_t num;
    printf(MENU_NORMALOPER);
    while (True) {
        printf("input(1~3)>>");
        fflush(stdout);
        scanf("%d", &num);

        switch (num) {
            case 1: do_normal_query(fd, userhd); break;
            case 2: do_normal_change(fd, userhd); goto print_menu;
            case 3: do_logout(fd, userhd); return;
        }
        continue;
    print_menu:
        printf(MENU_NORMALOPER);         //打印普通用户菜单
    }
}

void do_admin_query(int fd, struct userhandle* userhd)
{
    uint32_t num = 0, count;
    struct msg_search* msg = (struct msg_search*)userhd->msg;
    printf(MENU_ADMINQUERY);
    while (True) {
        printf("input(1~3)>>");
        fflush(stdout);
        scanf("%d", &num);
        while (num == 0 || num > 3) {
            printf("输入错误,请重新输入!\ninput(1~3)>>");
            fflush(stdout);
            scanf("%d", &num);
        }
        if (num == 3) {
            break;
        } else if (num == 1) {
            printf("请输入您要查找的用户名: ");
            fflush(stdout);
            scanf("%s", msg->name);
            while (getchar() != '\n');
            msg->msg.cmdcode = CLI_SEARCH_NAME;
        } else if (num == 2) {
            msg->msg.cmdcode = CLI_SEARCH_ALL;
        }
        msg->id = userhd->id;
        msg->usertype = ADMINISTRATOR;
        msg->msg.size = sizeof(struct msg_search);

        count = 0;
        send(fd, (void*)msg, msg->msg.size, 0);
        while (True) {
            if (recv_msg(fd, (struct msg*)msg) == 0) do_server_exit(fd);
            if (msg->msg.cmdcode == SER_ACKEND) break;
            if (count == 0) printf("工号\t用户类型\t姓名\t密码\t年龄\t电话\t地址\t职位\t入职年月\t工资\t等级\n");
            if (msg->msg.cmdcode == SER_ACK) {
                count++;
                printf("=========================================================================================================\n");
                printf("%d,\t", ((struct msg_userinfo*)msg)->userinfo.id);
                printf("%d,\t", ((struct msg_userinfo*)msg)->userinfo.usertype);
                printf("%s\t,", ((struct msg_userinfo*)msg)->userinfo.name);
                printf("%s,\t", ((struct msg_userinfo*)msg)->userinfo.password);
                printf("%d,\t", ((struct msg_userinfo*)msg)->userinfo.age);
                printf("%s,\t", ((struct msg_userinfo*)msg)->userinfo.contact);
                printf("%s,\t", ((struct msg_userinfo*)msg)->userinfo.address);
                printf("%s,\t", ((struct msg_userinfo*)msg)->userinfo.office);
                printf("%s,\t", ((struct msg_userinfo*)msg)->userinfo.date);
                printf("%d,\t", ((struct msg_userinfo*)msg)->userinfo.pay);
                printf("%d\n", ((struct msg_userinfo*)msg)->userinfo.level);
            }
        }
        if (count == 0) {
            printf("=========================================================================================================\n");
            printf("查找为空...\n");
        }
    }
}

void do_normal_query(int fd, struct userhandle* userhd)
{
    struct msg_search* msg = (struct msg_search*)userhd->msg;

    msg->id = userhd->id;
    msg->usertype = NORMAL;
    msg->msg.cmdcode = CLI_SEARCH_OWN;
    msg->msg.size = sizeof(struct msg_search);

    send(fd, (void*)msg, msg->msg.size, 0);
    printf("工号	用户类型	 姓名	密码	年龄	电话	地址	职位	入职年月	工资	 等级\n");
    while (True) {
        if (recv_msg(fd, (struct msg*)msg) == 0) do_server_exit(fd);
        if (msg->msg.cmdcode == SER_ACKEND) break;

        if (msg->msg.cmdcode == SER_ACK) {
            printf("=========================================================================================================\n");
            printf("%d,  ", ((struct msg_userinfo*)msg)->userinfo.id);
            printf("%d,  ", ((struct msg_userinfo*)msg)->userinfo.usertype);
            printf("%s,  ", ((struct msg_userinfo*)msg)->userinfo.name);
            printf("%s,  ", ((struct msg_userinfo*)msg)->userinfo.password);
            printf("%d,  ", ((struct msg_userinfo*)msg)->userinfo.age);
            printf("%s,  ", ((struct msg_userinfo*)msg)->userinfo.contact);
            printf("%s,  ", ((struct msg_userinfo*)msg)->userinfo.address);
            printf("%s,  ", ((struct msg_userinfo*)msg)->userinfo.office);
            printf("%s,  ", ((struct msg_userinfo*)msg)->userinfo.date);
            printf("%d,  ", ((struct msg_userinfo*)msg)->userinfo.pay);
            printf("%d\n", ((struct msg_userinfo*)msg)->userinfo.level);
        }
    }
}

void do_admin_change(int fd, struct userhandle* userhd)
{
    uint32_t num = 0;
    struct msg_change* msg = (struct msg_change*)userhd->msg;
    static enum entry entrylst[] = { STAFF_NAME, STAFF_AGE, STAFF_ADDRRESS, STAFF_CONTACT, STAFF_OFFICE, STAFF_PAY, STAFF_DATE, STAFF_LEVEL, STAFF_PASSWORD };
    static const char* menulst[] = { "姓名", "年龄", "家庭地址", "电话", "职位", "工资", "入职日期", "评级", "密码" };

    printf("请输入要修改的职员ID: "); fflush(stdout);
    scanf("%d", &((struct msg_change*)userhd->msg)->chid);            //填充被修改用户ID

    printf(MENU_ADMINCHANGE);
    printf("修改职员ID: %d\n", msg->chid);
    while (True) {
        printf("input(1~10)>>");
        fflush(stdout);
        scanf("%d", &num);
        while (num == 0 || num > 10) {
            printf("输入错误,请重新输入!\ninput(1~10)>>");
            fflush(stdout);
            scanf("%d", &num);
        }
        if (num == 10) break;

        printf("请输入%s: ", menulst[num - 1]);
        fflush(stdout);
        scanf("%s", msg->value);            //填充修改值value(以字符串形式)

        msg->key = entrylst[num - 1];       //填充被修改项key
        msg->id = userhd->id;               //填充操作用户ID
        msg->usertype = userhd->usertype;   //填充用户类型usertype
        msg->msg.cmdcode = CLI_CHANGE;      //填充命令码cmdtype
        msg->msg.size = sizeof(struct msg_change);  //填充数据包大小size

        send(fd, (void*)msg, msg->msg.size, 0);
        if (recv_msg(fd, (struct msg*)msg) == 0) do_server_exit(fd);  //服务器掉线
        if (msg->msg.cmdcode == SER_SUCCESS) {
            printf("数据库修改成功!\n");
        } else if (msg->msg.cmdcode == SER_ERR) {                   //操作错误
            printf("%s!\n", deal_errcode((struct msg_ack*)msg));
        }
    }
}

void do_normal_change(int fd, struct userhandle* userhd)
{
    uint32_t num = 0;
    struct msg_change* msg = (struct msg_change*)userhd->msg;
    static enum entry entrylst[] = { STAFF_ADDRRESS, STAFF_CONTACT, STAFF_PASSWORD };
    static const char* menulst[] = { "家庭地址", "电话", "密码" };

    printf(MENU_NORMALCHANGE);
    msg->chid = userhd->id;                 //填充被修改用户ID
    printf("修改职员ID: %d\n", msg->chid);

    while (True) {
        printf("input(1~4)>>");
        fflush(stdout);
        scanf("%d", &num);
        while (num == 0 || num > 4) {
            printf("输入错误,请重新输入!\ninput(1~4)>>");
            fflush(stdout);
            scanf("%d", &num);
        }
        if (num == 4) break;

        printf("请输入%s: ", menulst[num - 1]);
        fflush(stdout);
        scanf("%s", msg->value);            //填充修改值value(以字符串形式)
    
        msg->key = entrylst[num - 1];       //填充被修改项key
        msg->id = userhd->id;               //填充操作用户ID
        msg->usertype = userhd->usertype;   //填充用户类型usertype
        msg->msg.cmdcode = CLI_CHANGE;      //填充命令码cmdtype
        msg->msg.size = sizeof(struct msg_change);  //填充数据包大小size

        send(fd, (void*)msg, msg->msg.size, 0);
        if (recv_msg(fd, (struct msg*)msg) == 0) do_server_exit(fd);  //服务器掉线
        if (msg->msg.cmdcode == SER_SUCCESS) {
            printf("数据库修改成功!\n");
        } else if (msg->msg.cmdcode == SER_ERR) {                   //操作错误
            printf("%s!\n", deal_errcode((struct msg_ack*)msg));
        }
    }
}

void do_admin_adduser(int fd, struct userhandle* userhd)
{
    unsigned char ret;
    struct msg_insert* msg = (struct msg_insert*)userhd->msg;
    while (True) {
        printf("\ec***************热烈欢迎新员工***************\n");
        do {
            if (ret != '\n') while (getchar() != '\n');
            printf("请输入新员工ID: "); fflush(stdout);
            scanf("%d", &msg->userinfo.id); while (getchar() != '\n');
            printf("您输入的工号是: %d\n", msg->userinfo.id);
            printf("工号信息一旦录入无法更改,请确认您所输入的是否正确!(Y[default]/N): "); fflush(stdout);
            ret = getchar();
        } while ((ret | 32) == 'N');
        if (ret != '\n') while (getchar() != '\n');

        printf("请输入姓名: "); fflush(stdout);
        scanf("%s", msg->userinfo.name);

        printf("请输入用户密码: "); fflush(stdout);
        scanf("%s", msg->userinfo.password);

        printf("请输入年龄: "); fflush(stdout);
        scanf("%d", &msg->userinfo.age);

        printf("请输入联系电话: "); fflush(stdout);
        scanf("%s", msg->userinfo.contact); while (getchar() != '\n');

        printf("请输入家庭住址: "); fflush(stdout);
        scanf("%s", msg->userinfo.address); while (getchar() != '\n');

        printf("请输入职位: "); fflush(stdout);
        scanf("%s", msg->userinfo.office); while (getchar() != '\n');

        printf("请输入入职日期(格式:0000.00.00): "); fflush(stdout);
        scanf("%s", msg->userinfo.date); while (getchar() != '\n');

        printf("请输入评级(1~5,5为最高,新员工为 1): "); fflush(stdout);
        scanf("%d", &msg->userinfo.level); while (getchar() != '\n');

        printf("请输入工资: "); fflush(stdout);
        scanf("%d", &msg->userinfo.pay); while (getchar() != '\n');

        printf("是否为管理员(Y/N[default]):"); fflush(stdout);

        if (((ret = getchar()) | 32) == 'Y') msg->userinfo.usertype = ADMINISTRATOR;
        else msg->userinfo.usertype = NORMAL;
        if (ret != '\n') while (getchar() != '\n');

        msg->id = userhd->id;
        msg->usertype = userhd->usertype;
        msg->msg.cmdcode = CLI_INSERT;
        msg->msg.size = sizeof(struct msg_insert);
        send(fd, (void*)msg, msg->msg.size, 0);
        if (recv_msg(fd, (struct msg*)msg) == 0) do_server_exit(fd);
        if (msg->msg.cmdcode == SER_SUCCESS) {
            printf("员工添加成功!");
        } else {
            printf("员工添加失败!%s!", deal_errcode((struct msg_ack*)msg));
        }
        printf("是否继续添加员工(Y/N[default]):");

        if (((ret = getchar()) | 32) == 'Y') {
            if (ret != '\n') while (getchar() != '\n');
            continue;
        }
        if (ret != '\n') while (getchar() != '\n');
        break;
    }
}

void do_admin_deluser(int fd, struct userhandle* userhd)
{
    struct msg_delete* msg = (struct msg_delete*)userhd->msg;
    printf("请输入要删除的用户ID: "); fflush(stdout);
    scanf("%d", &msg->chid);

    msg->id = userhd->id;
    msg->usertype = userhd->usertype;
    msg->msg.size = sizeof(struct msg_delete);
    msg->msg.cmdcode = CLI_DELETE;

    send(fd, (void*)msg, msg->msg.size, 0);
    if (recv_msg(fd, (struct msg*)msg) == 0) do_server_exit(fd);
    if (msg->msg.cmdcode == SER_SUCCESS) {
        printf("删除用户成功!删除用户ID: %d\n", msg->chid);
    } else {
        printf("数据库修改失败!%s!\n", deal_errcode((struct msg_ack*)msg));
    }
}

void do_admin_history(int fd, struct userhandle* userhd)
{
    uint32_t count = 0;
    unsigned char strbuf[32];
    struct msg_search* msg = (struct msg_search*)userhd->msg;

    msg->id = userhd->id;
    msg->usertype = userhd->usertype;
    msg->msg.cmdcode = CLI_SEARCH_HISTORY;
    msg->msg.size = sizeof(struct msg_search);
    send(fd, (void*)msg, msg->msg.size, 0);

    count = 0;
    while (True) {
        if (recv_msg(fd, (struct msg*)msg) == 0) do_server_exit(fd);
        if (msg->msg.cmdcode == SER_ACKEND) break;

        if (msg->msg.cmdcode == SER_ACK) {
            printf("%s---", getLocalTime(((struct msg_historylog*)msg)->log.time, strbuf));
            printf("%d---", ((struct msg_historylog*)msg)->log.id);
            printf("%s.\n", ((struct msg_historylog*)msg)->log.event);
            count++;
        }
    }
    if (count == 0) {
        printf("无历史记录!\n");
    }
}