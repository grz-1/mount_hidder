#define TARGET_PATH_NUM 3
#define MAX_PATH_LEN 80

struct seq_file {
	char *buf;
	size_t size;
	size_t from;
	size_t count;
	size_t pad_until;
};