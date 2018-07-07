#include <gmimepp.hpp>
#include <iostream>


int GmimePP::init()
{
    if (!isFileExist())
        return -1;

    g_mime_init(GMIME_ENABLE_RFC2047_WORKAROUNDS);

    int fd;
    if ((fd = open(m_mailPath.c_str(), O_RDWR)) == -1) {
        return -1;
    }

    GMimeStream *gstream = g_mime_stream_fs_new (fd);
    GMimeParser *gparser = g_mime_parser_new ();
    g_mime_parser_init_with_stream (gparser, gstream);
    m_gmessage = g_mime_parser_construct_message (gparser);

    if (!m_gmessage)
        return -1;

    g_object_unref (gstream);
    g_object_unref (gparser);
    m_fds.push_back(fd);

    return 0;
}

int GmimePP::getHeader(const std::string &headerName, std::string &value) const
{
    const char *val = g_mime_object_get_header((GMimeObject *) m_gmessage, headerName.c_str());

    if (val == nullptr) {
        return -1;
    }

    char* decodedValue = g_mime_utils_header_decode_text(val);
    value.assign(decodedValue);
    g_free(decodedValue);
    return 0;
}

std::string GmimePP::getHeader(const std::string &headerName) const
{
    std::string ret;
    const char *val = g_mime_object_get_header((GMimeObject *) m_gmessage, headerName.c_str());

    if (val == nullptr) {
        return "";
    }

    char* decodedValue = g_mime_utils_header_decode_text(val);
    ret.assign(decodedValue);
    g_free(decodedValue);
    return ret;
}


int  GmimePP::getHeaders(std::vector<SHeaderValue> &vHeaderValuePairs) const
{
    const char *name = nullptr, *value = nullptr;
    GMimeHeaderIter it;

    GMimeHeaderList *headerList = g_mime_object_get_header_list(((GMimeObject *) m_gmessage));
    if (!headerList)
        return -1;

    if (!g_mime_header_list_get_iter (headerList, &it))
        return -1;

    do {
        if (!g_mime_header_iter_is_valid (&it))
                break;
        name = g_mime_header_iter_get_name (&it);
        value = g_mime_header_iter_get_value (&it);

        SHeaderValue m {(name != NULL) ? name : "", (value != NULL) ? value : ""};
        vHeaderValuePairs.push_back(m);

    } while(g_mime_header_iter_next (&it));

    return 0;
}

int GmimePP::addHeader(const std::string &header, const std::string &value)
{
    int fd = open((m_mailPath + ".copy").c_str(), O_CREAT | O_RDWR);
    if (fd < 0) {
        return -1;
    }

    char* encValue = g_mime_utils_header_encode_text(value.c_str());
    g_mime_object_append_header(GMIME_OBJECT (m_gmessage), header.c_str(), encValue);

    GMimeStream *gstream = g_mime_stream_fs_new(fd);
    g_mime_stream_fs_set_owner (GMIME_STREAM_FS(gstream), FALSE);

    if (g_mime_object_write_to_stream (GMIME_OBJECT (m_gmessage), gstream) < 0) {
        g_free(encValue);
        g_object_unref (gstream);
        close(fd);
        return -1;
    }

    g_mime_stream_flush (gstream);
    g_object_unref (gstream);
    g_free(encValue);
    m_fds.push_back(fd);

    std::remove(m_mailPath.c_str());
    std::rename((m_mailPath + ".copy").c_str(), m_mailPath.c_str());
    return 0;
}


int GmimePP::setHeader(const std::string &header, const std::string &newValue)
{
    int fd = open((m_mailPath + ".copy").c_str(), O_CREAT | O_RDWR);
    if (fd < 0) {
        return -1;
    }

    char* encValue = g_mime_utils_header_encode_text(newValue.c_str());

    g_mime_object_set_header(GMIME_OBJECT (m_gmessage), header.c_str(), encValue);
    GMimeStream *gstream = g_mime_stream_fs_new(fd);
    g_mime_stream_fs_set_owner (GMIME_STREAM_FS(gstream), FALSE);

    if (g_mime_object_write_to_stream (GMIME_OBJECT (m_gmessage), gstream) < 0) {
        g_free(encValue);
        g_object_unref (gstream);
        close(fd);
        return -1;
    }

    g_mime_stream_flush (gstream);
    g_object_unref (gstream);
    g_free(encValue);
    m_fds.push_back(fd);

    std::remove(m_mailPath.c_str());
    std::rename((m_mailPath + ".copy").c_str(), m_mailPath.c_str());
    return 0;
}

std::string GmimePP::getFromAdress() const
{
  std::string ret;

  const char *nameAndAdress = g_mime_message_get_sender(m_gmessage);
  InternetAddressList *list = internet_address_list_parse_string (nameAndAdress);

  if (list) {
      InternetAddress *address = internet_address_list_get_address (list, 0);
      if (address) {
          InternetAddressMailbox *mailbox;
          mailbox = INTERNET_ADDRESS_MAILBOX (address);
          ret.assign(internet_address_mailbox_get_addr(mailbox));
          g_object_unref(list);
          return ret;
      }
  }

  g_object_unref(list);
  return ret;
}

int GmimePP::getRecipientsByType(GMimeRecipientType type, std::vector<SHeaderValue> &vRes) const
{
	const char *name = nullptr, *value = nullptr;

	InternetAddressList *rcptsList = g_mime_message_get_recipients (m_gmessage, type);

  if (rcptsList == nullptr)
      return 0;

	int size = internet_address_list_length(rcptsList);

	for(int i = 0; i < size; i++) {
		InternetAddress *t = internet_address_list_get_address (rcptsList, i);
		name = internet_address_get_name (t);

		if (INTERNET_ADDRESS_IS_MAILBOX(t)) {
			value = internet_address_mailbox_get_addr(INTERNET_ADDRESS_MAILBOX(t));
		}
    if (value != nullptr)
        vRes.push_back(SHeaderValue({name == nullptr ? "" : name, value}));
	}

	return 0;
}

int GmimePP::getAllRecipients(std::vector<SHeaderValue> &vRes) const
{
	const char *name = nullptr, *value = nullptr;

	InternetAddressList *rcptsList = g_mime_message_get_all_recipients (m_gmessage);

	if (rcptsList == nullptr)
		return -1;

	int size = internet_address_list_length(rcptsList);

	for(int i = 0; i < size; i++) {
		InternetAddress *t = internet_address_list_get_address (rcptsList, i);
		name = internet_address_get_name (t);

		if (INTERNET_ADDRESS_IS_MAILBOX(t)) {
			value = internet_address_mailbox_get_addr(INTERNET_ADDRESS_MAILBOX(t));
                        if (value != nullptr)
                            vRes.push_back(SHeaderValue({name == nullptr ? "" : name, value}));
		}
	}

  g_object_unref(rcptsList);
	return 0;
}

int GmimePP::saveAttachments(const std::string &path) const
{
    GMimePartIter *iter = g_mime_part_iter_new((GMimeObject *)m_gmessage);

    do
    {
        GMimeObject *part = g_mime_part_iter_get_current (iter);

        GMimeContentDisposition *disp = nullptr;
        disp = g_mime_object_get_content_disposition(part);

        if ((disp != nullptr) &&
            (!g_ascii_strcasecmp(disp->disposition, "attachment"))){

                char *aname = (char *) g_mime_object_get_content_disposition_parameter(part,
                                                                                       "filename");
                if (aname == nullptr)
                    aname = (char *) g_mime_object_get_content_type_parameter(part, "name");

                if (aname == nullptr)
                    continue;

                char out[1024] = { '\0' };
                snprintf(out, sizeof(out) - 1, "%s/%s", path.c_str(), aname);

                int fd;
                if ((fd = open (out, O_CREAT | O_WRONLY, 0755)) < 0)
                    return -1;

        	      GMimeStream *stream = g_mime_stream_fs_new (fd);

              	GMimeDataWrapper *content = g_mime_part_get_content_object ((GMimePart *)part);
              	g_mime_stream_flush (stream);

        	      g_object_unref (stream);
                close(fd);
        }

    }
    while(g_mime_part_iter_next (iter));

    g_mime_part_iter_free(iter);

    return 0;
}

std::string GmimePP::getBodyHTML() const
{
      //to be implemented
      return "";
}

std::string GmimePP::getBodyText() const
{
    //to be implemented
    return "";
}
