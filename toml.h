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

#ifndef TOML_H
#define TOML_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct toml_table_t toml_table_t;
typedef struct toml_array_t toml_array_t;

/* Parse a TOML string into a table. Returns NULL on error.
 * errbuf must be at least 200 bytes long.
 */
toml_table_t *toml_parse(char *conf, char *errbuf, int errbufsz);
toml_table_t *toml_parse_file(FILE *fp, char *errbuf, int errbufsz);

void toml_free(toml_table_t *pt);

/* Table queries */
int toml_key_in(const toml_table_t *tab, const char *key);
const char *toml_key_at(const toml_table_t *tab, int idx);

/* Get raw string for a key. Returns 0 on success. */
int toml_string_in(const toml_table_t *tab, const char *key, char **val);
int toml_bool_in(const toml_table_t *tab, const char *key, int *val);
int toml_int_in(const toml_table_t *tab, const char *key, int64_t *val);
int toml_double_in(const toml_table_t *tab, const char *key, double *val);
toml_table_t *toml_table_in(const toml_table_t *tab, const char *key);
toml_table_t *toml_subtable_at(const toml_table_t *tab, const char *key, int idx);
toml_array_t *toml_array_in(const toml_table_t *tab, const char *key);

/* Array queries */
int toml_array_nelem(const toml_array_t *arr);
int toml_string_at(const toml_array_t *arr, int idx, char **val);
int toml_bool_at(const toml_array_t *arr, int idx, int *val);
int toml_int_at(const toml_array_t *arr, int idx, int64_t *val);
int toml_double_at(const toml_array_t *arr, int idx, double *val);
toml_table_t *toml_table_at(const toml_array_t *arr, int idx);
toml_array_t *toml_array_at(const toml_array_t *arr, int idx);

#ifdef __cplusplus
}
#endif

#endif /* TOML_H */
