// bigfile entry
typedef struct
{
	unsigned int hash; // hashed name
	unsigned int size; // file size
	unsigned int offset; // file offset
}
bigfileentry_t;

// bigfile header
typedef struct
{
		bigfileentry_t *entries;
		unsigned int	numentries;
}
bigfileheader_t;