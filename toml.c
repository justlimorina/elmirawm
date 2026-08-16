/*
  MIT License

  Copyright (c) 2017 CK Tan
  https://github.com/cktan/tomlc99

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include "toml.h"

typedef struct toml_pair_t toml_pair_t;

struct toml_pair_t {
	char *key;
	char *val;
};

struct toml_table_t {
	char *key;
	int nkval;
	toml_pair_t **kval;
	int ntab;
	toml_table_t **tab;
	int narr;
	toml_array_t **arr;
};

struct toml_array_t {
	char *key;
	int kind; /* 'v'alue, 'a'rray, 't'able */
	int type; /* 's'tring, 'i'nt, 'd'ouble, 'b me'ool, 't'ime */
	int nelem;
	char **val;
	toml_array_t **arr;
	toml_table_t **tab;
};

static char *strdup_safe(const char *s) {
	char *p;
	if (!s) return NULL;
	p = malloc(strlen(s) + 1);
	if (p) strcpy(p, s);
	return p;
}

static void set_err(char *errbuf, int errbufsz, const char *msg) {
	if (errbuf && errbufsz > 0) {
		snprintf(errbuf, errbufsz, "%s", msg);
	}
}

static char *skip_ws(char *p) {
	while (*p && (*p == ' ' || *p == '\t' || *p == '\r')) p++;
	return p;
}

static void free_pair(toml_pair_t *p) {
	if (!p) return;
	if (p->key) free(p->key);
	if (p->val) free(p->val);
	free(p);
}

static void free_array(toml_array_t *arr);

void toml_free(toml_table_t *pt) {
	int i;
	if (!pt) return;
	if (pt->key) free(pt->key);
	for (i = 0; i < pt->nkval; i++) free_pair(pt->kval[i]);
	if (pt->kval) free(pt->kval);
	for (i = 0; i < pt->ntab; i++) toml_free(pt->tab[i]);
	if (pt->tab) free(pt->tab);
	for (i = 0; i < pt->narr; i++) free_array(pt->arr[i]);
	if (pt->arr) free(pt->arr);
	free(pt);
}

static void free_array(toml_array_t *arr) {
	int i;
	if (!arr) return;
	if (arr->key) free(arr->key);
	if (arr->val) {
		for (i = 0; i < arr->nelem; i++) if (arr->val[i]) free(arr->val[i]);
		free(arr->val);
	}
	if (arr->arr) {
		for (i = 0; i < arr->nelem; i++) free_array(arr->arr[i]);
		free(arr->arr);
	}
	if (arr->tab) {
		for (i = 0; i < arr->nelem; i++) toml_free(arr->tab[i]);
		free(arr->tab);
	}
	free(arr);
}

static toml_table_t *create_table(const char *key) {
	toml_table_t *t = calloc(1, sizeof(*t));
	if (t && key) t->key = strdup_safe(key);
	return t;
}

static toml_pair_t *create_pair(const char *key, const char *val) {
	toml_pair_t *p = calloc(1, sizeof(*p));
	if (p) {
		p->key = strdup_safe(key);
		p->val = strdup_safe(val);
	}
	return p;
}

static void add_pair(toml_table_t *tab, toml_pair_t *p) {
	tab->kval = realloc(tab->kval, (tab->nkval + 1) * sizeof(*tab->kval));
	tab->kval[tab->nkval++] = p;
}

static void add_subtable(toml_table_t *tab, toml_table_t *sub) {
	tab->tab = realloc(tab->tab, (tab->ntab + 1) * sizeof(*tab->tab));
	tab->tab[tab->ntab++] = sub;
}

static void add_subarray(toml_table_t *tab, toml_array_t *arr) {
	tab->arr = realloc(tab->arr, (tab->narr + 1) * sizeof(*tab->arr));
	tab->arr[tab->narr++] = arr;
}

toml_table_t *toml_parse(char *conf, char *errbuf, int errbufsz) {
	toml_table_t *root;
	toml_table_t *cur_tab;
	char *line;

	if (!conf) return NULL;
	root = create_table(NULL);
	cur_tab = root;
	line = conf;

	while (*line) {
		char *next_line = strchr(line, '\n');
		char *p;
		if (next_line) *next_line = '\0';

		p = skip_ws(line);
		if (*p == '#' || *p == '\0') {
			if (next_line) line = next_line + 1; else break;
			continue;
		}

		if (*p == '[') {
			/* Section header */
			char *end;
			p++;
			if (*p == '[') p++;
			end = strchr(p, ']');
			if (end) {
				char *sec;
				char *send;
				toml_table_t *sub;

				*end = '\0';
				sec = skip_ws(p);
				send = sec + strlen(sec) - 1;
				while (send > sec && (*send == ' ' || *send == '\t' || *send == ']')) *send-- = '\0';

				sub = create_table(sec);
				add_subtable(root, sub);
				cur_tab = sub;
			}
		} else {
			/* Key-Value pair */
			char *eq = strchr(p, '=');
			if (eq) {
				char *key;
				char *kend;
				char *val;
				char *vend;

				*eq = '\0';
				key = skip_ws(p);
				kend = key + strlen(key) - 1;
				while (kend > key && (*kend == ' ' || *kend == '\t')) *kend-- = '\0';

				val = skip_ws(eq + 1);
				vend = val + strlen(val) - 1;
				while (vend > val && (*vend == ' ' || *vend == '\t' || *vend == '\r')) *vend-- = '\0';

				/* Check array */
				if (*val == '[') {
					char *arr_buf = strdup_safe(val);
					char *closing = strchr(arr_buf, ']');
					char *aptr;
					toml_array_t *arr = calloc(1, sizeof(*arr));
					arr->key = strdup_safe(key);
					arr->kind = 'v';

					/* Accumulate multiline array content until closing ']' */
					while (!closing && next_line) {
						char *nl;
						size_t cur_len, add_len;
						line = next_line + 1;
						next_line = strchr(line, '\n');
						if (next_line) *next_line = '\0';

						nl = skip_ws(line);
						cur_len = strlen(arr_buf);
						add_len = strlen(nl);
						arr_buf = realloc(arr_buf, cur_len + add_len + 2);
						arr_buf[cur_len] = ' ';
						strcpy(arr_buf + cur_len + 1, nl);
						closing = strchr(arr_buf, ']');
					}

					if (closing) *closing = '\0';

					/* Extract string elements from array buffer */
					aptr = arr_buf + 1; /* skip '[' */
					while (*aptr) {
						char quote = 0;
						char *start;
						size_t el_len;

						aptr = skip_ws(aptr);
						if (!*aptr || *aptr == ']') break;
						if (*aptr == '#') {
							/* comment inside array, skip to end of string or line */
							while (*aptr && *aptr != '\n') aptr++;
							continue;
						}
						if (*aptr == ',') {
							aptr++;
							continue;
						}

						if (*aptr == '"' || *aptr == '\'') {
							quote = *aptr;
							aptr++;
						}

						start = aptr;
						if (quote) {
							while (*aptr && *aptr != quote) {
								if (*aptr == '\\' && *(aptr + 1)) aptr += 2;
								else aptr++;
							}
						} else {
							while (*aptr && *aptr != ',' && *aptr != ']' && *aptr != ' ' && *aptr != '\t' && *aptr != '\r' && *aptr != '\n') {
								aptr++;
							}
						}

						el_len = aptr - start;
						if (quote && *aptr == quote) aptr++;

						if (el_len > 0) {
							char *item = malloc(el_len + 1);
							memcpy(item, start, el_len);
							item[el_len] = '\0';
							arr->val = realloc(arr->val, (arr->nelem + 1) * sizeof(*arr->val));
							arr->val[arr->nelem++] = item;
						}
					}
					free(arr_buf);
					add_subarray(cur_tab, arr);
				} else {
					/* Normal value */
					toml_pair_t *pair;
					if ((*val == '"' && *vend == '"') || (*val == '\'' && *vend == '\'')) {
						val++;
						*vend = '\0';
					}
					pair = create_pair(key, val);
					add_pair(cur_tab, pair);
				}
			}
		}

		if (next_line) line = next_line + 1; else break;
	}

	return root;
}

toml_table_t *toml_parse_file(FILE *fp, char *errbuf, int errbufsz) {
	long sz;
	char *buf;
	toml_table_t *t;

	if (!fp) {
		set_err(errbuf, errbufsz, "null file pointer");
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	sz = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (sz <= 0) {
		set_err(errbuf, errbufsz, "empty file");
		return NULL;
	}

	buf = malloc(sz + 1);
	if (!buf) return NULL;
	fread(buf, 1, sz, fp);
	buf[sz] = '\0';

	t = toml_parse(buf, errbuf, errbufsz);
	free(buf);
	return t;
}

int toml_key_in(const toml_table_t *tab, const char *key) {
	int i;
	if (!tab || !key) return 0;
	for (i = 0; i < tab->nkval; i++) {
		if (strcmp(tab->kval[i]->key, key) == 0) return 1;
	}
	return 0;
}

int toml_string_in(const toml_table_t *tab, const char *key, char **val) {
	int i;
	if (!tab || !key || !val) return -1;
	for (i = 0; i < tab->nkval; i++) {
		if (strcmp(tab->kval[i]->key, key) == 0) {
			*val = strdup_safe(tab->kval[i]->val);
			return 0;
		}
	}
	return -1;
}

int toml_bool_in(const toml_table_t *tab, const char *key, int *val) {
	int i;
	if (!tab || !key || !val) return -1;
	for (i = 0; i < tab->nkval; i++) {
		if (strcmp(tab->kval[i]->key, key) == 0) {
			if (strcmp(tab->kval[i]->val, "true") == 0 || strcmp(tab->kval[i]->val, "1") == 0) {
				*val = 1;
				return 0;
			} else if (strcmp(tab->kval[i]->val, "false") == 0 || strcmp(tab->kval[i]->val, "0") == 0) {
				*val = 0;
				return 0;
			}
		}
	}
	return -1;
}

int toml_int_in(const toml_table_t *tab, const char *key, int64_t *val) {
	int i;
	if (!tab || !key || !val) return -1;
	for (i = 0; i < tab->nkval; i++) {
		if (strcmp(tab->kval[i]->key, key) == 0) {
			*val = strtoll(tab->kval[i]->val, NULL, 10);
			return 0;
		}
	}
	return -1;
}

toml_table_t *toml_table_in(const toml_table_t *tab, const char *key) {
	int i;
	if (!tab || !key) return NULL;
	for (i = 0; i < tab->ntab; i++) {
		if (tab->tab[i]->key && strcmp(tab->tab[i]->key, key) == 0) {
			return tab->tab[i];
		}
	}
	return NULL;
}

toml_table_t *toml_subtable_at(const toml_table_t *tab, const char *key, int idx) {
	int i, count = 0;
	if (!tab || !key || idx < 0) return NULL;
	for (i = 0; i < tab->ntab; i++) {
		if (tab->tab[i]->key && strcmp(tab->tab[i]->key, key) == 0) {
			if (count == idx) return tab->tab[i];
			count++;
		}
	}
	return NULL;
}

toml_array_t *toml_array_in(const toml_table_t *tab, const char *key) {
	int i;
	if (!tab || !key) return NULL;
	for (i = 0; i < tab->narr; i++) {
		if (tab->arr[i]->key && strcmp(tab->arr[i]->key, key) == 0) {
			return tab->arr[i];
		}
	}
	return NULL;
}

int toml_array_nelem(const toml_array_t *arr) {
	return arr ? arr->nelem : 0;
}

int toml_string_at(const toml_array_t *arr, int idx, char **val) {
	if (!arr || idx < 0 || idx >= arr->nelem || !val) return -1;
	*val = strdup_safe(arr->val[idx]);
	return 0;
}
