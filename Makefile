CC=g++
FLAGS=-Wall -std=c++11 -pedantic
SERV=server
CLNT=client

all : $(SERV) $(CLNT)

$(SERV) : $(SERV).cpp common.cpp
	$(CC) $(FLAGS) -pthread $(SERV).cpp common.cpp -o $(SERV)

$(CLNT) : $(CLNT).cpp common.cpp
	$(CC) $(FLAGS) $(CLNT).cpp common.cpp -o $(CLNT)


PORT=5577
LISTEN=./$(SERV) $(PORT)
SEND=./$(CLNT) localhost $(PORT)
SERVOUT=result.txt
CLNTOUT=[1-6].txt
INT32_MIN=-2147483648
INT32_MAX=2147483647

test : all
	@rm -f $(SERVOUT) $(CLNTOUT)
	$(LISTEN) & sleep 1
	$(SEND) 1 3 9 7 5 >1.txt &
	$(SEND) 94584 11111 77 222 >2.txt &
	$(SEND) 333 >3.txt &
	$(SEND) -2 -1 1 -9 >4.txt &
	$(SEND) $(INT32_MAX) $(INT32_MIN) >5.txt &
	$(SEND) $(INT32_MAX) $(INT32_MAX) $(INT32_MAX) $(INT32_MAX) >6.txt &
	sleep 1
	@echo -e '\nGot:\n[clients]'
	@cat $(CLNTOUT)
	@echo '[server]'
	@cat $(SERVOUT)
	@echo -e '\nExpected:\n[clients]\n5\n26499\n333\n-3\n-1\n2147483647\n[server]\n429502046'


clean :
	@rm -f $(SERV) $(CLNT) $(SERVOUT) $(CLNTOUT)
