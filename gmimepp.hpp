/*
/*  GMimePP
 *  Copyright (C) 2016 Islam YASAR
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 */


#ifndef GMIMEPP_HPP
#define GMIMEPP_HPP

#include <string>
#include <gmime/gmime.h>
#include <vector>
#include <sys/stat.h>
#include <fcntl.h>
#include <fstream>
#include <cstdio>

struct SHeaderValue
{
    std::string name;
    std::string value;
};


class GmimePP
{
public:
    GmimePP(const std::string &mailPath) : m_mailPath(mailPath){};

    virtual ~GmimePP()
    {
      g_mime_shutdown();
      g_object_unref(m_gmessage);
      for (auto fd : m_fds)
          close(fd);
    };

public:
    int init();
    int getHeader(const std::string &header, std::string &value) const;
    std::string getHeader(const std::string &header) const;
    int addHeader(const std::string &header, const std::string &value);
    int setHeader(const std::string &header, const std::string &newValue);
    int getHeaders(std::vector<SHeaderValue> &) const;
    std::string getFromAdress() const;
    std::string getBodyHTML() const;
    std::string getBodyText() const;
    int getRecipientsByType(GMimeRecipientType, std::vector<SHeaderValue> &) const;
    int getAllRecipients(std::vector<SHeaderValue> &) const;
    int saveAttachments(const std::string&) const;

private:
  GmimePP(const GmimePP& orig) = delete;

  inline bool isFileExist ()
  {
    struct stat buffer;
    return (stat (m_mailPath.c_str(), &buffer) == 0);
  }

private:
    std::string     m_mailPath;
    GMimeMessage   *m_gmessage;
    std::vector<int> m_fds;
};

#endif /* GMIMEPP_HPP */
