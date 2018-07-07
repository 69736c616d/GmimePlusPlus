#include <gmimepp.hpp>
#include <iostream>

using namespace std;



int main(int argc, char const *argv[])
{

  //while(true)
  //{
      GmimePP gpp(argv[1]);

      gpp.init();
      cout << "From: " << gpp.getHeader("FROM") << endl;
      cout << "To: " << gpp.getHeader("To") << endl;
      cout << "Subject: " << gpp.getHeader("Subject") << endl;
      cout << "Message-ID: " << gpp.getHeader("Message-ID") << endl;

      gpp.addHeader("header1", "value1");

      gpp.setHeader("header1", "value2");


      std::vector<SHeaderValue> allrcpts;
      gpp.getAllRecipients(allrcpts);

      for (auto &rcpt : allrcpts)
          cout << "allrcpt" << rcpt.name << "-" << rcpt.value << endl;

      allrcpts.clear();

      /*
          GMIME_RECIPIENT_TYPE_TO,
          GMIME_RECIPIENT_TYPE_CC,
          GMIME_RECIPIENT_TYPE_BCC
      */
      gpp.getRecipientsByType(GMIME_RECIPIENT_TYPE_TO, allrcpts);

      for (auto &rcpt : allrcpts)
          cout << "rcptbytype" <<rcpt.name << "-" << rcpt.value << endl;

      cout << gpp.getFromAdress() << endl;

      //getchar();
      //sleep(0.1);

  //}

      //gpp.saveAttachments("/home/iyasar/gmimetest/GmimePlusPlus");


  return 0;
}
