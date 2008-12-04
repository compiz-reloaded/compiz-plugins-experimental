/*
 * Compiz cube model plugin
 *
 * cubemodel.c
 *
 * This plugin displays wavefront (.obj) 3D mesh models inside of
 * the transparent cube.
 *
 * Copyright : (C) 2008 by David Mikos
 * E-mail    : infiniteloopcounter@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*
 * A collection of file parsing methods for fast extraction of text
 * in a file.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "cubemodel-internal.h"

fileParser *
initFileParser (FILE *fp,
                int  bufferSize)
{
    fileParser *fParser = malloc (sizeof(fileParser));
    if (!fParser)
	return NULL;

    fParser->fp              = fp;
    fParser->bufferSize      = bufferSize;
    fParser->cp              = bufferSize;
    fParser->oldStrline      = NULL;
    fParser->lastTokenOnLine = FALSE;
    fParser->buf             = malloc (sizeof (char) * bufferSize);
    if (!fParser->buf)
	freeFileParser (fParser);

    return fParser;
}

/**********************************************************
* updateFileParser:                                       *
* Start parsing from new file pointer.                    *
* Might be useful in rewinding or changing to a new file. *
**********************************************************/

void
updateFileParser (fileParser *fParser,
                  FILE *fp)
{
    fParser->fp         = fp;
    fParser->cp         = fParser->bufferSize;
    fParser->lastTokenOnLine = FALSE;
}

void
freeFileParser (fileParser * fParser)
{
    if (fParser)
    {
	if (fParser->buf)
	    free (fParser->buf);
	if (fParser->oldStrline)
	    free (fParser->oldStrline);
	free (fParser);
    }
    fParser = NULL;
}


/****************************************************
 * getLine:                                         *
 * Optimized to run fast, this will return a string *
 * that forms the next line of text in a file.      *
 ***************************************************/

char *
getLine (fileParser * fParser)
{
    FILE *fp          = fParser->fp;
    char **oldStrline = &(fParser->oldStrline);
    char *buf         = fParser->buf;
    int  bufferSize   = fParser->bufferSize;
    int  *cp          = &(fParser->cp);

    fParser->lastTokenOnLine = FALSE;

    int  i;
    int  readChars = bufferSize;
    int  size = 0;
    char *strline;

    if (*cp >= bufferSize)
    {
	if (feof (fp))
	    return NULL;

	*cp = 0;

	readChars = fread (buf, sizeof (char), bufferSize, fp);

	if (readChars < bufferSize)
	    buf[readChars]='\0';
    }

    if (buf[*cp] == '\0') /* make sure no new line after EOF */
	return NULL;

    for (i = *cp; i < readChars; i++)
    {
	if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\0')
	{
	    int cp2 = *cp;

	    *cp = i + 1;
	    if (buf[i] == '\0')
		*cp = bufferSize;

	    buf[i] = '\0';

	    return &(buf[cp2]);
	}
    }

    if (readChars < bufferSize)
    {
	int cp2 = *cp;

	buf[readChars] = '\0';
	*cp = bufferSize;
	return &(buf[cp2]);
    }

    /* Read half a line from buffer. Now copy to a new string,
     * read in more data into the buffer and combine until end of line.
     */

    do
    {
	int remainder = readChars - *cp;

	strline = realloc (*oldStrline, sizeof (char) * (size + remainder));
	*oldStrline = strline;

	memcpy (&strline[size], &buf[*cp], sizeof (char) * remainder);
	*cp = 0;
	size += remainder;

	readChars = fread (buf, sizeof (char), bufferSize, fp);

	if (readChars < bufferSize)
	    buf[readChars]='\0';

	for (i = 0; i < readChars; i++)
	{
	    if (buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\0')
	    {
		strline= realloc (*oldStrline, sizeof (char) * (size + i + 1));
		*oldStrline = strline;

		memcpy (&strline[size], buf, sizeof (char) * i);

		strline[size + i] = '\0';
		*cp = i + 1;
		if (buf[i] == '\0')
		    *cp = bufferSize;

		return strline;
	    }
	}

	if (readChars < bufferSize)
	{
	    strline = realloc(*oldStrline,
			      sizeof (char) * (size +readChars + 1));

	    *oldStrline = strline;
	    memcpy (&strline[size], buf, sizeof (char) * readChars);

	    strline[size + readChars] = '\0';

	    *cp = bufferSize;

	    return strline;
	}
    }
    while (!feof (fp));

    /* should never get here */

    return NULL;
}

/****************************************************
 * getLineToken:                                    *
 * Returns next token (string separated by delims,  *
 * tab and whitespace) on the current line of text. *
 * If none are found the next line is checked.      *
 * Returns NULL when there are no more tokens.      *
 ***************************************************/

char *
getLineToken (fileParser * fParser)
{
    FILE *fp          = fParser->fp;
    char **oldStrline = &(fParser->oldStrline);
    char *buf         = fParser->buf;
    int  bufferSize   = fParser->bufferSize;
    int  *cp          = &(fParser->cp);
    int cp2;

    int  i;
    int  readChars = bufferSize;
    int  size = 0;
    char *strline;

    fParser->lastTokenOnLine = TRUE;

    if (*cp >= bufferSize)
    {
	if (feof (fp))
	    return NULL;

	*cp = 0;

	readChars = fread (buf, sizeof (char), bufferSize, fp);

	if (readChars < bufferSize)
	    buf[readChars]='\0';

	if (readChars == 0 && feof (fp)) /* for one delim at end of file */
	{
	    *cp = bufferSize;
	    return buf;
	}
    }

    if (buf[*cp] == '\0') /* make sure no new line after EOF */
	return NULL;

    for (i = *cp; i < readChars; i++)
    {
	switch (buf[i])
	{
	case ' '  :
	case '\t' :
	    fParser->lastTokenOnLine = FALSE;
	    if (i + 1 < bufferSize)
	    {
		if (buf[i+1]=='\0')
		{
		    cp2 = *cp;
		    *cp = bufferSize - 1;
		    buf[*cp] = ' ';

		    buf[i] = '\0';

		    return &(buf[cp2]);
		}
	    }
	    else if (feof (fp))
		fParser->lastTokenOnLine = TRUE;

	case '\n' :
	case '\r' :
	case '\0' :
	    cp2 = *cp;

	    *cp = i + 1;
	    if (buf[i] == '\0')
		*cp = bufferSize;

	    buf[i] = '\0';

	    return &(buf[cp2]);
	}
    }

    if (readChars < bufferSize)
    {
	cp2 = *cp;

	buf[readChars] = '\0';
	*cp = bufferSize;
	return &(buf[cp2]);
    }

    /* Read half a line from buffer. Now copy to a new string,
     * read in more data into the buffer and combine until end of line.
     */

    do
    {
	int remainder = readChars - *cp;

	strline = realloc (*oldStrline, sizeof (char) * (size + remainder));
	*oldStrline = strline;

	memcpy (&strline[size], &buf[*cp], sizeof (char) * remainder);
	*cp = 0;
	size += remainder;

	readChars = fread (buf, sizeof (char), bufferSize, fp);

	if (readChars < bufferSize)
	    buf[readChars]='\0';

	for (i = 0; i < readChars; i++)
	{
	    switch (buf[i])
	    {
	    case ' '  :
	    case '\t' :
		fParser->lastTokenOnLine = FALSE;

		    if (i + 1 < bufferSize)
		    {
			if (buf[i + 1] == '\0')
			{
			    strline= realloc (*oldStrline, sizeof (char) * (size + i + 1));
			    *oldStrline = strline;

			    memcpy (&strline[size], buf, sizeof (char) * i);

			    strline[size + i] = '\0';

			    *cp = bufferSize - 1;
			    buf[*cp] = ' ';

			    return strline;
			}
		    }
		    else if (feof (fp))
			fParser->lastTokenOnLine = TRUE;

	    case '\n' :
	    case '\r' :
	    case '\0' :
		strline= realloc (*oldStrline, sizeof (char) * (size + i + 1));
		*oldStrline = strline;

		memcpy (&strline[size], buf, sizeof (char) * i);

		strline[size + i] = '\0';
		*cp = i + 1;
		if (buf[i] == '\0')
		    *cp = bufferSize;

		return strline;
	    }
	}

	if (readChars < bufferSize)
	{
	    strline = realloc(*oldStrline,
			      sizeof (char) * (size +readChars + 1));

	    *oldStrline = strline;
	    memcpy (&strline[size], buf, sizeof (char) * readChars);

	    strline[size + readChars] = '\0';

	    *cp = bufferSize;

	    return strline;
	}
    }
    while (!feof (fp));

    /* should never get here */

    return NULL;
}


/****************************************************************
 * getLineToken2:                                               *
 *  similar to getLineToken, but with some differences          *
 * - does't return "" results (i.e. skip contiguous delims),    *
 *   except at end of line where it may return "" once.         *
 * - returns NULL when sameLineTokens is true and there are no  *
 *   more tokens on that line                                   *
 ***************************************************************/

char *
getLineToken2 (fileParser * fParser,
               Bool sameLineTokens)
{
    if (sameLineTokens && fParser->lastTokenOnLine)
	return NULL;

    char * strline = getLineToken (fParser);

    if (!strline)
	return NULL;

    while (strline[0] == '\0')
    {
	if (fParser->lastTokenOnLine)
	{
	    if (sameLineTokens)
		return NULL;
	    return strline;
	}
	strline = getLineToken (fParser);
	if (!strline)
	{
	    if (sameLineTokens)
		return NULL;
	    return "\0";
	}
    }

    return strline;
}

/***********************************************
* skipLine:                                    *
* Skips to the next line of text in the file.  *
* Maybe useful where some tokens are found and *
* no more are sought on the current line.      *
***********************************************/

void
skipLine (fileParser * fParser)
{
    FILE *fp          = fParser->fp;
    char *buf         = fParser->buf;
    int  bufferSize   = fParser->bufferSize;
    int  *cp          = &(fParser->cp);

    fParser->lastTokenOnLine = FALSE;

    int  i;
    int  readChars = bufferSize;

    do
    {
	if (*cp >= bufferSize)
	{
	    if (feof (fp))
		return;

	    *cp = 0;

	    readChars = fread (buf, sizeof (char), bufferSize, fp);

	    if (readChars < bufferSize)
		buf[readChars]='\0';
	}

	if (buf[*cp] == '\0') /* make sure no new line after EOF */
	    return;

	for (i = *cp; i < readChars; i++)
	{
	    switch (buf[i])
	    {
	    case '\n' :
	    case '\r' :
		*cp = i + 1;
		return;
	    case '\0' :
		*cp = bufferSize;
		return;
	    }
	}

	*cp = bufferSize; /* because need to get new line into buffer */

	if (readChars < bufferSize)
	    return;

    } while (!feof (fp));

    return;
}

/******************************
* strsep2:                    *
* Has a similar effect to     *
* strsep except it skips      *
* contiguous delim characters *
******************************/

char *
strsep2 (char       **strPtr,
	 const char *delim)
{
    char *tmpStr;

    if (!strPtr || !delim)
	return NULL;

    tmpStr = strsep (strPtr, delim);
    if (!tmpStr)
	return NULL;

    while (*strPtr)
    {
	if (tmpStr && tmpStr[0] == '\0')
	    tmpStr = strsep(strPtr, delim);
	else
	    break;
    }

    return tmpStr;
}

