
/*
* SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
*
* SPDX-License-Identifier: Apache-2.0
*/


#include "SyncDirException.h"


//
// Implementation: Class SyncDirException
//


// Default constructor ==> assign NULL string data member.

SyncDirException::SyncDirException(): msg("")
{}


// Constructor with exception message ==> copy message to data member.

SyncDirException::SyncDirException(const std::string &exmsg): msg(exmsg) 
{}


// Implement virtual member function of std::exception class.

const char* SyncDirException::what() const noexcept
{
  return msg.c_str();
}



