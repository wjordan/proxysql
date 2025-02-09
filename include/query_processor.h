#ifndef __CLASS_QUERY_PROCESSOR_H
#define __CLASS_QUERY_PROCESSOR_H
#include "proxysql.h"
#include "cpp.h"

#define FAST_ROUTING_NEW208


#ifdef FAST_ROUTING_NEW208
#include "khash.h"
KHASH_MAP_INIT_STR(khStrInt, int)
#endif
#define PROXYSQL_QPRO_PTHREAD_MUTEX

typedef std::unordered_map<std::uint64_t, void *> umap_query_digest;
typedef std::unordered_map<std::uint64_t, char *> umap_query_digest_text;

struct _Query_Processor_rule_t {
	int rule_id;
	bool active;
	char *username;
	char *schemaname;
	int flagIN;
	char *client_addr;
	int client_addr_wildcard_position;
	char *proxy_addr;
	int proxy_port;
	uint64_t digest;
	char *match_digest;
	char *match_pattern;
	bool negate_match_pattern;
	int re_modifiers; // note: this is passed as char*, but converted to bitsfield
	int flagOUT;
	char *replace_pattern;
	int destination_hostgroup;
	int cache_ttl;
	int cache_empty_result;
	int cache_timeout;
	int reconnect;
	int timeout;
	int retries;
	int delay;
	int next_query_flagIN;
	int mirror_hostgroup;
	int mirror_flagOUT;
	char *error_msg;
	char *OK_msg;
	int sticky_conn;
	int multiplex;
	int gtid_from_hostgroup;
	int log;
	bool apply;
  char *comment; // #643
	void *regex_engine1;
	void *regex_engine2;
	uint64_t hits;
	struct _Query_Processor_rule_t *parent; // pointer to parent, to speed up parent update
};

typedef struct _Query_Processor_rule_t QP_rule_t;

class Query_Processor_Output {
	public:
	void *ptr;
	unsigned int size;
	int destination_hostgroup;
	int mirror_hostgroup;
	int mirror_flagOUT;
	int next_query_flagIN;
	int cache_ttl;
	int cache_empty_result;
	int cache_timeout;
	int reconnect;
	int timeout;
	int retries;
	int delay;
	char *error_msg;
	char *OK_msg;
	int sticky_conn;
	int multiplex;
	int gtid_from_hostgroup;
	long long max_lag_ms;
	int log;
	char *comment; // #643
	std::string *new_query;
	void * operator new(size_t size) {
		return l_alloc(size);
	}
	void operator delete(void *ptr) {
		l_free(sizeof(Query_Processor_Output),ptr);
	}
	Query_Processor_Output() {
		init();
	}
	~Query_Processor_Output() {
		destroy();
	}
	void init() {
		ptr=NULL;
		size=0;
		destination_hostgroup=-1;
		mirror_hostgroup=-1;
		mirror_flagOUT=-1;
		next_query_flagIN=-1;
		cache_ttl=-1;
		cache_empty_result=1;
		cache_timeout=-1;
		reconnect=-1;
		timeout=-1;
		retries=-1;
		delay=-1;
		sticky_conn=-1;
		multiplex=-1;
		gtid_from_hostgroup=-1;
		max_lag_ms=-1;
		log=-1;
		new_query=NULL;
		error_msg=NULL;
		OK_msg=NULL;
		comment=NULL; // #643
	}
	void destroy() {
		if (error_msg) {
			free(error_msg);
			error_msg=NULL;
		}
		if (OK_msg) {
			free(OK_msg);
			OK_msg=NULL;
		}
		if (comment) { // #643
			free(comment);
		}
		if (comment) { // #643
			free(comment);
		}
	}
};

static char *commands_counters_desc[MYSQL_COM_QUERY___NONE];

class Command_Counter {
	private:
	int cmd_idx;
	int _add_idx(unsigned long long t) {
		if (t<=100) return 0;
		if (t<=500) return 1;
		if (t<=1000) return 2;
		if (t<=5000) return 3;
		if (t<=10000) return 4;
		if (t<=50000) return 5;
		if (t<=100000) return 6;
		if (t<=500000) return 7;
		if (t<=1000000) return 8;
		if (t<=5000000) return 9;
		if (t<=10000000) return 10;
		return 11;
	}
	public:
	unsigned long long total_time;
	unsigned long long counters[13];
	Command_Counter(int a) {
		total_time=0;
		cmd_idx=a;
		total_time=0;
		for (int i=0; i<13; i++) {
			counters[i]=0;
		}
	}
	unsigned long long add_time(unsigned long long t) {
		total_time+=t;
		counters[0]++;
		int i=_add_idx(t);
		counters[i+1]++;
		return total_time;
	}
	char **get_row() {
		char **pta=(char **)malloc(sizeof(char *)*15);
		pta[0]=commands_counters_desc[cmd_idx];
		itostr(pta[1],total_time);
		for (int i=0;i<13;i++) itostr(pta[i+2], counters[i]);
		return pta;
	}
	void free_row(char **pta) {
		for (int i=1;i<15;i++) free(pta[i]);
		free(pta);
	}
};

class Query_Processor {
	private:
	char rand_del[16];
	umap_query_digest digest_umap;
	umap_query_digest_text digest_text_umap;
#ifdef PROXYSQL_QPRO_PTHREAD_MUTEX
	pthread_rwlock_t digest_rwlock;
#else
	rwlock_t digest_rwlock;
	int64_t padding;	// to get rwlock cache aligned
#endif
	enum MYSQL_COM_QUERY_command __query_parser_command_type(SQP_par_t *qp);
	protected:
#ifdef PROXYSQL_QPRO_PTHREAD_MUTEX
	pthread_rwlock_t rwlock;
#else
	rwlock_t rwlock;
#endif
	std::vector<QP_rule_t *> rules;
#ifdef FAST_ROUTING_NEW208
	khash_t(khStrInt) * rules_fast_routing;
	char * rules_fast_routing___keys_values;
	unsigned long long rules_fast_routing___keys_values___size;
#else
	std::unordered_map<std::string,int> rules_fast_routing;
#endif
	Command_Counter * commands_counters[MYSQL_COM_QUERY___NONE];
	volatile unsigned int version;
	unsigned long long rules_mem_used;
	public:
	Query_Processor();
	~Query_Processor();
	void print_version();
	void reset_all(bool lock=true);
	void wrlock();		// explicit write lock, to be used in multi-isert 
	void wrunlock();	// explicit write unlock
	bool insert(QP_rule_t *qr, bool lock=true);		// insert a new rule. Uses a generic void pointer to a structure that may vary depending from the Query Processor
	QP_rule_t * new_query_rule(int rule_id, bool active, char *username, char *schemaname, int flagIN, char *client_addr, char *proxy_addr, int proxy_port, char *digest, char *match_digest, char *match_pattern, bool negate_match_pattern, char *re_modifiers, int flagOUT, char *replace_pattern, int destination_hostgroup, int cache_ttl, int cache_empty_result, int cache_timeout, int reconnect, int timeout, int retries, int delay, int next_query_flagIN, int mirror_hostgroup, int mirror_flagOUT, char *error_msg, char *OK_msg, int sticky_conn, int multiplex, int gtid_from_hostgroup, int log, bool apply, char *comment);	// to use a generic query rule struct, this is generated by this function and returned as generic void pointer
	void delete_query_rule(QP_rule_t *qr);	// destructor
	Query_Processor_Output * process_mysql_query(MySQL_Session *sess, void *ptr, unsigned int size, Query_Info *qi);
	void delete_QP_out(Query_Processor_Output *o);

	void sort(bool lock=true);

	void init_thread();
	void end_thread();
	void commit();	// this applies all the changes in memory
	SQLite3_result * get_current_query_rules();
	SQLite3_result * get_stats_query_rules();	

	void update_query_processor_stats();

	void query_parser_init(SQP_par_t *qp, char *query, int query_length, int flags);
	enum MYSQL_COM_QUERY_command query_parser_command_type(SQP_par_t *qp);
	bool query_parser_first_comment(Query_Processor_Output *qpo, char *fc);
	void query_parser_free(SQP_par_t *qp);
	char * get_digest_text(SQP_par_t *qp);
	uint64_t get_digest(SQP_par_t *qp);

	void update_query_digest(SQP_par_t *qp, int hid, MySQL_Connection_userinfo *ui, unsigned long long t, unsigned long long n, MySQL_STMT_Global_info *_stmt_info, MySQL_Session *sess);

	unsigned long long query_parser_update_counters(MySQL_Session *sess, enum MYSQL_COM_QUERY_command c, SQP_par_t *qp, unsigned long long t);

	SQLite3_result * get_stats_commands_counters();
	SQLite3_result * get_query_digests();
	SQLite3_result * get_query_digests_reset();
	unsigned long long purge_query_digests(bool async_purge, bool parallel, char **msg);
	unsigned long long purge_query_digests_async(char **msg);
	unsigned long long purge_query_digests_sync(bool parallel);

	unsigned long long get_query_digests_total_size();
	unsigned long long get_rules_mem_used();


	// fast routing
	SQLite3_result * fast_routing_resultset;
	void load_fast_routing(SQLite3_result *resultset);
	SQLite3_result * get_current_query_rules_fast_routing();
	int testing___find_HG_in_mysql_query_rules_fast_routing(char *username, char *schemaname, int flagIN);
};

typedef Query_Processor * create_Query_Processor_t();

#endif /* __CLASS_QUERY_PROCESSOR_H */
