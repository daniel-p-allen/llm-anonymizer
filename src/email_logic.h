#ifndef EMAIL_LOGIC_H
#define EMAIL_LOGIC_H

#include <string>

class EmailLogic {
public:
    // Returns false if retrieval failed, so the caller can stop rather than
    // carry on with whatever data.json happened to be left over.
    static bool processEmails(int num_emails);
};

#endif
