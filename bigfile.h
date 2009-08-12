// filetypes
typedef enum
{
	BIGENTRY_UNKNOWN, // unknown data
	BIGENTRY_TIM, // TIM texture
	BIGENTRY_VAG // PSX audio file
}bigentrytype_t;

// filetype extensions
static char *bigentryext[3] = 
{ 
	".dat", 
	".tim", 
	".vag" 
};

// bigfile entry
typedef struct
{
	unsigned int hash; // hashed name
	unsigned int size; // file size
	unsigned int offset; // file offset
	char name[16]; // only of loaded from 
	bigentrytype_t type;
}
bigfileentry_t;

// bigfile header
typedef struct
{
		bigfileentry_t *entries;
		unsigned int	numentries;
}
bigfileheader_t;