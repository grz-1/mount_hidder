#define TARGET_PATH_NUM 2
#define MAX_PATH_LEN 100

struct seq_file {
	char *buf;
	size_t size;
	size_t from;
	size_t count;
	size_t pad_until;
};