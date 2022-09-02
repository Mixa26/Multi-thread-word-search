#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dirent.h>

#include <time.h>
//#include <conio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>

#include <unistd.h>

#include <pthread.h>

#define COMMANDSIZE 30
#define MAXFILES 1000

#define MAX_WORD_LEN 64 //najveca dozvoljena duzina reci, uz \0
#define LETTERS 26 //broj slova u abecedi i broj dece u trie

typedef struct search_result //rezultat pretrage
{
	int result_count; //duzina niza
	char **words; //niz stringova, svaki string duzine MAX_WORD_LEN
} search_result;

search_result* create_result(int len)
{
	search_result* temp = (search_result*)malloc(sizeof(search_result));
	temp->result_count = len;
	//temp->words = (char**)malloc(len * MAX_WORD_LEN);
	//printf("LEN: %d\n",len);
	temp->words = (char**)malloc(len * MAX_WORD_LEN);
	for (int i = 0; i < len; i++)
	{
		*(temp->words+i) = (char*)malloc(MAX_WORD_LEN);
	}
	return temp;
} 

typedef struct scanned_file //datoteka koju je scanner vec skenirao
{
	char name[256]; //naziv datoteke
	char time[100]; //vreme poslednje modifikacije datoteke
} scanned_file;

scanned_file* create_scanned_file()
{
	scanned_file* temp = (scanned_file*)malloc(sizeof(scanned_file));
	temp->name[0] = '\0';
	temp->time[0] = '\0';
	return temp;
}

typedef struct trie_node //cvor unutar trie strukture
{
	char c; //slovo ovog cvora
	int term; //flag za kraj reci
	int subwords; //broj reci u podstablu, ne racunajuci sebe
	struct trie_node *parent; //pokazivac ka roditelju
	struct trie_node *children[LETTERS]; //deca
	pthread_mutex_t lock;
} trie_node;

trie_node* create_trie()
{
	trie_node* temp = (trie_node*)malloc(sizeof(trie_node));
	temp->c = '0';
	temp->term = 0;
	temp->subwords = 0;
	temp->parent = NULL;
	int i;
	for (i = 0; i < LETTERS; i++)
	{
		temp->children[i] = NULL;
	}
	pthread_mutex_init(&temp->lock,NULL);
	return temp;
}

//deljeni niz fajlova koji su procitani
scanned_file scanned_files[MAXFILES];
//poslednji skeniran je last_scanned - 1 zapravo
int last_scanned = 0;

trie_node* root;

search_result* result;

int last_subwords = 0;

int search = 0;

char prefSearch[MAX_WORD_LEN];

////////////////////////////////////////////////////
//FUNKCIJE

int prefix(const char *pre, const char *str)
{
	if (strncmp(pre, str, strlen(pre)) == 0)
		return 1;
	return 0;
}

trie_node* find_prefix_end(trie_node* root, char *prefix, int i)
{
	if (prefix[i] == '\0')
	{
		//printf("%d\n", root->subwords);
		return root;
	}
	//for (int j = 0; j < LETTERS; j++)
	//{
	int j = prefix[i]-'a';
		//if (root->children[j] != NULL)
		//	printf("%c\n", root->children[j]->c);
		if (root->children[j] != NULL && root->children[j]->c == prefix[i])
		{
			return find_prefix_end(root->children[j], prefix, i+1);
		}
	//}
	return NULL;
}

//i se prosledjuje po referenci
void fill_result_with_words(trie_node* prefix, char temp[MAX_WORD_LEN], int* i, int g, search_result* result)
{
	if (*i >= result->result_count)
	{
		return;
	}
	if (prefix->term == 1)
	{
		temp[g] = '\0';
		//printf("%s %d\n", temp, *i);
		
		strcpy(result->words[*i],temp);
		(*i)++;
	}
	for (int j = 0; j < LETTERS; j++)
	{
		if (prefix->children[j] != NULL)
		{
			temp[g] = prefix->children[j]->c;
			fill_result_with_words(prefix->children[j], temp, i, g+1, result);
		}
	}
}

void print_all()
{
	pthread_mutex_lock(&root->lock);
	for (int i = 0; i < LETTERS; i++)
		if (root->children[i] != NULL)
			printf("%c %d\n", root->children[i]->c, root->children[i]->subwords);
	pthread_mutex_unlock(&root->lock);
}

void trie_free_result(search_result *result)
{
	for (int i = 0; i < result->result_count; i++)
	{
		free(*(result->words+i));
	}
	free(result->words);

	free(result);
	result = NULL;
}

//search_result*
search_result* trie_get_words(char *prefix) //operacija za pretragu
{
	trie_node* temp = root;
	trie_node* temp1;
	trie_node* currLock = NULL;
	//search_result* result = NULL;	

	int j = prefix[0]-'a';
	if (j >= 26 || j < 0)
	{
		result = create_result(0);
		last_subwords = 0;
		return result;
	}
	if (root->children[j] != NULL && root->children[j]->c == prefix[0])
	{
		currLock = root->children[j];
		pthread_mutex_lock(&root->children[j]->lock);
	}
	
	temp1 = find_prefix_end(temp, prefix, 0);
	//printf("%c\n",temp1->c);
	//pravimo rezultat sa nizom velicine podreci trie_node
	if (temp1 != NULL)
	{
		//printf("%d\n",temp1->subwords);
		result = create_result(temp1->subwords);
		//result = create_result(1000);
		last_subwords = temp1->subwords;
	}
	else
	{
		result = create_result(0);
		last_subwords = 0;
	}
	char word[MAX_WORD_LEN];
	strcpy(word,prefix);
	int k = 0;
	while(word[k] != '\0')
		k++;
	int i = 0;
	if (temp1 != NULL)
	{
		fill_result_with_words(temp1, word, &i, k, result);
	}
	if (currLock != NULL)
		pthread_mutex_unlock(&currLock->lock);
	return result;
}

void add_subwords(trie_node* bottom)
{
	if (bottom == NULL)return;
	bottom->subwords++;
	add_subwords(bottom->parent);
}

void trie_add_word(char *word)
{
	trie_node* temp = root;
	int i = 0;
	int l = 0;
	trie_node* nodeLocked = NULL;
	//printf("%s ", word);
	
	while (1)
	{
		i = word[l]-'a';
		if (temp->children[i] != NULL)
		{
			//if (strcmp(word,"axin") == 0)
			//{
			//	printf("%c\n", temp->children[i]->c);
			//}
			if (temp->children[i]->c == word[l])
			{
				//printf("%c\n", word[l]);
				if (l == 0)
				{
					nodeLocked = temp->children[i];
					//printf("%s\n zakljucan ",word);
					pthread_mutex_lock(&temp->children[i]->lock);	
				}
				if (word[l+1] == '\0')
					temp->children[i]->term = 1;
				temp = temp->children[i];
				//i = 0;			
				l++;
			}
		}
		else
		{
			temp->children[i] = create_trie();
			temp->children[i]->c = word[l];
			//printf("%c", temp->children[i]->c);
			//printf("%d\n", i);
			temp->children[i]->parent = temp;
			if (l == 0)
				nodeLocked = temp->children[i];
			if (word[l+1] == '\0')
			{
				//printf(" ");
				temp->children[i]->term = 1;
				if (nodeLocked != NULL)
				{
					pthread_mutex_unlock(&nodeLocked->lock);
					nodeLocked = NULL;
					//printf("otkljucan\n");
				}
				if (search == 1 && prefix(prefSearch,word) == 1)
					printf("%s\n", word);
				add_subwords(temp->children[i]);
				return;
			}
			temp = temp->children[i];
			//i = 0;
			l++;
		}
		if (word[l] == '\0' && nodeLocked != NULL)
		{
			pthread_mutex_unlock(&nodeLocked->lock);
			nodeLocked = NULL;
			return;
			//printf("otkljucan\n");
		}
	}
}

int is_regular_file(const char *path)
{
	//vraca samo obicne fajlove, ukoliko je direktorijum ili nes drugo vraca false
	struct stat path_stat;
	stat(path, &path_stat);
	//potencijalan izvor problema
	//printf("%d\n",S_ISDIR(path_stat.st_mode));
	//printf("%s\n",path);
	return !S_ISDIR(path_stat.st_mode);
}

void get_last_modified(char t[100], char* file)
{
	struct stat attr;
	
	//printf("FILE: %s\n",file);
	stat(file, &attr);

	strcpy(t, ctime(&attr.st_mtime));
}

int should_scan(char name[256])
{
	//1 treba da skeniras
	//0 ne skeniraj ponovo
	//skeniras fajl ukoliko se ne nalazi u scanned_files
	//ili ukoliko mu se last modified razlikuje od onog u scanned_files 
	//(znaci da je menjan)	
	int i;

	for (i = 0; i < last_scanned; i++)
	{
		if (strcmp(scanned_files[i].name, name) == 0)
		{
			char temp[100];
			get_last_modified(temp,scanned_files[i].name);
			if (strcmp(scanned_files[i].time,temp) == 0)
			{
				//nije modifikovan
				return 0;
			}
			else
			{
				//modifikovan je fajl
				//printf("name: %s\nscanned_f:%s temp:%s\n", scanned_files[i].name, scanned_files[i].time, temp);
				return 1;
			}
		}
	}
	//nije skeniran ni jednom
	return 1;
}

void scan_file(char* path)
{
	//prolazi kroz ceo fajl
	//uzima rec po red (identifikuje kraj kad dodje to ' ' ili '\t' ili '\n'
	//uzima max 63 karaktera	
	//ukoliko nadje bilo sta osim slova u reci nece je uzeti u obzir i ide na sledecu

	FILE *fp;

	char ch;

	//printf("%s\n", path);

	fp = fopen(path, "r");
	//fp = fopen("data-small/wiki4.txt","r");
	//fp = fopen("data-tiny/test.txt","r");

	if (fp == NULL)
	{
		printf("Error loading the file\n");
		return;
	}

	int i = 0;
	char temp[MAX_WORD_LEN];
	int invalid_char = 0;

	while ((ch = fgetc(fp)) != EOF)
	{
		//printf("%c", ch);
		if (ch == ' ' || ch == '\t' || ch == '\n')
		{
			temp[i] = '\0';
			if (invalid_char == 0)
			{	
				//printf("%s\n", temp);
				trie_add_word(temp);
			}
			invalid_char = 0;
			i = 0;
			continue;
		}
		if (invalid_char)continue;
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z'))
		{
			if (i < 63)
				temp[i++] = tolower(ch);
		}
		else
			invalid_char = 1;
	}
	fclose(fp);
}

void *scanner_work(void *_args)
{
	//zapravo scanner nit
	struct dirent *files;

	DIR *dir = opendir((char*) _args);
	if (dir == NULL)
	{
		printf("Directory cannot be opened!\n");
		return 0;
	}
	while (1)
	{	
		if ((files = readdir(dir)) == NULL)
		{		
			closedir(dir);
			//spavaj 5s pa ponovo citaj direktorijum
			sleep(5);			
			dir = opendir((char*) _args);
			if (dir == NULL)
			{
				printf("Directory cannot be opened!\n" );
				return 0;
			}
			continue;
		}
		else
		{
			char temp[MAX_WORD_LEN];
			strcpy(temp, (char*)_args);
			strcat(temp, "/");
			strcat(temp, files->d_name);
			strcpy(files->d_name, temp);
		}
		if (is_regular_file(files->d_name))
		{			
			if (should_scan(files->d_name) == 1)
			{
				//char temp[MAX_WORD_LEN];
				//strcpy(temp, (char*)_args);
				//strcat(temp, "/");
				//strcat(temp, files->d_name);
				scan_file(files->d_name);
				//printf("prosao\n");
				strcpy(scanned_files[last_scanned].name,files->d_name);
				//podesavanje last_modified 
				get_last_modified(scanned_files[last_scanned].time, files->d_name);
				//printf("obtain: %s\n", scanned_files[last_scanned].time);
				//get_last_modified(scanned_files[last_scanned].time, files->d_name);
				//printf("obtain: %s\n", scanned_files[last_scanned].time);
				last_scanned++;
			}
		}
	}
	closedir(dir);
}

int main()
{
	char command[COMMANDSIZE];
	char dir[COMMANDSIZE];

	root = create_trie();

	printf("Type in _add_ then enter to add a file for search.\n");
	printf("Type in _stop_ then enter to exit the program.\n");

	while (1)
	{
		fscanf(stdin,"%s",command);

		if (strcmp(command,"_add_") == 0)
		{
			//printf("%s\n", command);
			printf("Type in the directory you want to search then enter.\n");
			fscanf(stdin,"%s",dir);
			pthread_t thr;
			pthread_create(&thr, NULL, scanner_work, &dir);
		}
		else if (strcmp(command,"_stop_") == 0)
		{
			exit(0);
		}
		else
		{
			//print_all();		
			//search_result* result = trie_get_words(command);
			result = trie_get_words(command);
			//printf("problem\n");
			//printf("%d\n", result->result_count);
			for (int j = 0; j < result->result_count; j++)
			{
				printf("%s\n", result->words[j]);	
			}			
			strcpy(prefSearch,command);
			search = 1;
			char d[5];
			while(fgets(d,5,stdin)!=NULL);
			printf("Search terminated.\n");
			trie_free_result(result);
			search = 0;
		}
	}
	return 0;
}
