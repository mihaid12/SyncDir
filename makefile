
#
# SPDX-FileCopyrightText: Copyright © 2022 Mihai-Ioan Popescu <mihai.popescu.d12@gmail.com>
#
# SPDX-License-Identifier: Apache-2.0
#

INCDIR = include
OBJDIR = obj
BINDIR = bin
SRCDIR = src

CC1 = gcc
CC2 = g++
CFLAGS = -I. -I$(INCDIR) -Wall -Wextra -Wno-multichar -Wno-format-truncation -std=gnu11
CPPFLAGS = -I. -I$(INCDIR) -Wall -Wextra -Wno-multichar -Wno-format-truncation -std=c++11

_HEAD_CLT = syncdir_clt_def_types.h syncdir_essential_def_types.h SyncDirException.h syncdir_utile.h
HEAD_CLT = $(patsubst %,$(INCDIR)/%,$(_HEAD_CLT))									# lookup_pattern,replace_with,lookup_in_text

_HEAD_SRV = syncdir_srv_def_types.h syncdir_essential_def_types.h SyncDirException.h syncdir_utile.h
HEAD_SRV = $(patsubst %,$(INCDIR)/%,$(_HEAD_SRV))

_OBJ_CLT = syncdir_clt_data_transfer.o syncdir_clt_events.o syncdir_clt_file_info_proc.o syncdir_clt_main.o syncdir_clt_watch_manager.o \
 			syncdir_clt_watch_tree.o SyncDirException.o syncdir_utile.o
OBJ_CLT = $(patsubst %,$(OBJDIR)/%,$(_OBJ_CLT))

_OBJ_SRV = syncdir_srv_data_transfer.o syncdir_srv_hash_info_proc.o syncdir_srv_main.o SyncDirException.o syncdir_utile.o
OBJ_SRV = $(patsubst %,$(OBJDIR)/%,$(_OBJ_SRV))

OBJ = $(OBJ_CLT) $(OBJ_SRV)

BIN_CLT = $(BINDIR)/SyncDir_Client
BIN_SRV = $(BINDIR)/SyncDir_Server




make: server client

client: $(OBJ_CLT)
	$(CC2) -o $(BIN_CLT) $^ $(CPPFLAGS)									# $^: Get dependences from the "target:dep" rule

server: $(OBJ_SRV)
	$(CC2) -o $(BIN_SRV) $^ $(CPPFLAGS)


# SyncDir Client objects.

$(OBJDIR)/syncdir_clt_data_transfer.o : $(OBJDIR)/%.o : $(SRCDIR)/%.cpp $(INCDIR)/%.h $(HEAD_CLT)
	$(CC2) -c $< -o $@ $(CPPFLAGS)											# $<: Get first dep. $@: Get target.

$(OBJDIR)/syncdir_clt_events.o : $(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCDIR)/%.h $(HEAD_CLT) $(INCDIR)/syncdir_clt_file_info_proc.h \
	$(INCDIR)/syncdir_clt_watch_manager.h $(INCDIR)/syncdir_clt_watch_tree.h
	$(CC2) -c $< -o $@ $(CPPFLAGS)

$(OBJDIR)/syncdir_clt_file_info_proc.o : $(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCDIR)/%.h $(HEAD_CLT)
	$(CC2) -c $< -o $@ $(CPPFLAGS)

$(OBJDIR)/syncdir_clt_main.o : $(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h $(HEAD_CLT)
	$(CC1) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/syncdir_clt_watch_manager.o : $(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h $(HEAD_CLT)
	$(CC1) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/syncdir_clt_watch_tree.o : $(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCDIR)/%.h $(HEAD_CLT) $(INCDIR)/syncdir_clt_watch_manager.h
	$(CC2) -c $< -o $@ $(CPPFLAGS)



# SyncDir Server objects.

$(OBJDIR)/syncdir_srv_data_transfer.o : $(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCDIR)/%.h $(HEAD_SRV) $(INCDIR)/syncdir_srv_hash_info_proc.h
	$(CC2) -c $< -o $@ $(CPPFLAGS)

$(OBJDIR)/syncdir_srv_hash_info_proc.o : $(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCDIR)/%.h $(HEAD_SRV)
	$(CC2) -c $< -o $@ $(CPPFLAGS)

$(OBJDIR)/syncdir_srv_main.o : $(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCDIR)/%.h $(HEAD_SRV)
	$(CC2) -c $< -o $@ $(CPPFLAGS)



# SyncDir Client/Server common objects.

$(OBJDIR)/SyncDirException.o : $(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(INCDIR)/%.h
	$(CC2) -c $< -o $@ $(CPPFLAGS)

$(OBJDIR)/syncdir_utile.o : $(OBJDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/%.h $(INCDIR)/syncdir_essential_def_types.h
	$(CC1) -c $< -o $@ $(CFLAGS)



# Rule for clean.

.PHONY : clean																# Avoid as file: "clean" is no file.

clean : 
	rm -f $(BIN_CLT) $(BIN_SRV) $(OBJ)
