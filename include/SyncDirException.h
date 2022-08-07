
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#ifndef _SYNCDIREXCEPTION_H_
#define _SYNCDIREXCEPTION_H_
/*++
Header of the SyncDir own exception class, that implements std::exception.
--*/


#include <iostream>
#include <string>

//
// Class SyncDirException
//
class SyncDirException: public virtual std::exception
{
private:
    const std::string msg;
public:
    SyncDirException();
    SyncDirException(const std::string &exmsg);
    virtual const char* what() const noexcept;                          // noexcept, instead of throw(), since c++11 standard.
};



#endif //--> #ifndef _SYNCDIREXCEPTION_H_

