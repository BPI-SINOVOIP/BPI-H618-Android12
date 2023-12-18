/*
 * Copyright (c) 2008-2017 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : CdxMetaData.c
 * Description : Options of Meta data extracting...
 * History :
 *
 */

#include "CdxMetaData.h"
//#define LOG_TAG "CdxMetaData"

void SetMetaData(CdxMediaInfoT *mediaInfo, META_IDX idx, const char* content, char_enc encoding)
{
    if(!strcmp(content, "null"))
    {
        CDX_LOGV("do nothing.... idx : %d", idx);
        return ;
    }
    switch(idx)
    {
        case ARTIST:
            CDX_LOGV("ARTIST : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the ARTIST content length is larger than %d,maybe error!",
                    MAX_CONTENT_LEN);
                memcpy(mediaInfo->artist,content,MAX_CONTENT_LEN);
                mediaInfo->artistsz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->artist,content,strlen(content));
                mediaInfo->artistsz = strlen(content);
            }
            break;
        case ALBUM:
            CDX_LOGV("ALBUM : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the ALBUM content length is larger than %d,maybe error!",
                    MAX_CONTENT_LEN);
                memcpy(mediaInfo->album,content,MAX_CONTENT_LEN);
                mediaInfo->albumsz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->album,content,strlen(content));
                mediaInfo->albumsz = strlen(content);
            }
            mediaInfo->albumCharEncode = encoding;
            break;
        case ALBUM_ARTIST:
            CDX_LOGV("ALBUM_ARTIST : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the ALBUM_ARTIST content length is larger than %d,maybe error!",
                    MAX_CONTENT_LEN);
                memcpy(mediaInfo->albumArtist,content,MAX_CONTENT_LEN);
                mediaInfo->albumArtistsz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->albumArtist,content,strlen(content));
                mediaInfo->albumArtistsz = strlen(content);
            }
            break;
        case TITLE:
            CDX_LOGV("TITLE : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the TITLE content length is larger than %d,maybe error!",
                    MAX_CONTENT_LEN);
                memcpy(mediaInfo->title,content,MAX_CONTENT_LEN);
                mediaInfo->titlesz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->title,content,strlen(content));
                mediaInfo->titlesz = strlen(content);
            }
            mediaInfo->titleCharEncode = encoding;
            break;
        case COMPOSER:
            CDX_LOGV("COMPOSER : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the COMPOSER content length is larger than %d,maybe error!",
                    MAX_CONTENT_LEN);
                memcpy(mediaInfo->composer,content,MAX_CONTENT_LEN);
                mediaInfo->composersz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->composer,content,strlen(content));
                mediaInfo->composersz = strlen(content);
            }
            break;
        case GENRE:
            CDX_LOGV("GENRE : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the GENRE content length is larger than %d,maybe error!",
                    MAX_CONTENT_LEN);
                memcpy(mediaInfo->genre,content,MAX_CONTENT_LEN);
                mediaInfo->genresz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->genre,content,strlen(content));
                mediaInfo->genresz = strlen(content);
            }
            mediaInfo->genreCharEncode = encoding;
            break;
        case YEAR:
            CDX_LOGV("YEAR : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the YEAR content length is larger than %d,maybe error!",
                     MAX_CONTENT_LEN);
                memcpy(mediaInfo->year,content,MAX_CONTENT_LEN);
                mediaInfo->yearsz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->year,content,strlen(content));
                mediaInfo->yearsz = strlen(content);
            }
            mediaInfo->yearCharEncode = encoding;
            break;
        case AUTHOR:
            CDX_LOGV("AUTHOR : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the AUTHOR content length is larger than %d,maybe error!",
                    MAX_CONTENT_LEN);
                memcpy(mediaInfo->author,content,MAX_CONTENT_LEN);
                mediaInfo->authorsz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->author,content,strlen(content));
                mediaInfo->authorsz = strlen(content);
            }
            mediaInfo->authorCharEncode = encoding;
            break;
        case CDTRACKNUMBER:
            CDX_LOGV("CDTRACKNUMBER : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the CDTRACKNUMBER content length is larger than %d,maybe error!",
                     MAX_CONTENT_LEN);
                memcpy(mediaInfo->cdTrackNumber,content,MAX_CONTENT_LEN);
                mediaInfo->cdTrackNumbersz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->cdTrackNumber,content,strlen(content));
                mediaInfo->cdTrackNumbersz = strlen(content);
            }
            break;
        case DISCNUMBER:
            CDX_LOGV("To fixme .... DISCNUMBER");
            break;
        case COMPILATION:
            CDX_LOGV("COMPILATION : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the COMPILATION content length is larger than %d,maybe error!",
                     MAX_CONTENT_LEN);
                memcpy(mediaInfo->compilation,content,MAX_CONTENT_LEN);
                mediaInfo->compilationsz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->compilation,content,strlen(content));
                mediaInfo->compilationsz = strlen(content);
            }
            break;
        case DATE:
            CDX_LOGV("DATE : %s", content);
            CDX_LOGV("Strlen : %d", strlen(content));
            if(strlen(content) > MAX_CONTENT_LEN)
            {
                CDX_LOGW("the DATE content length is larger than %d,maybe error!",
                    MAX_CONTENT_LEN);
                memcpy(mediaInfo->date,content,MAX_CONTENT_LEN);
                mediaInfo->datesz = MAX_CONTENT_LEN;
            }
            else
            {
                memcpy(mediaInfo->date,content,strlen(content));
                mediaInfo->datesz = strlen(content);
            }
            break;
        default:
            CDX_LOGV("line : %d, content : %s", __LINE__, content);
            break;
    }
}

