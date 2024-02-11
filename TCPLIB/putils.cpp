#include <stdio.h>
#include <io.h>
#include <string.h>

/**
 * чтение из текстового файла массива
 * fileName - имя файла
 * buff - ссылка на массив куда будем читать
 * n - количество строк массива (сток в файле)
 * m - количество столбцов массива (столбцов в файле)
 */
void readMapFromFile(const char *fileName, char *buff, int n, int m) {
	FILE *fp;
	char s[33];

	fp = fopen(fileName, "r");
	if (fp != NULL) {
	   for (int i =0; i < n; i++) {
		   if (fgets(s, m + 1, fp) != NULL) {
			   s[m - 1] = '\0';
			   strcpy(&buff[i * m], s);
		   }
		}
	   fclose(fp);
	}
}