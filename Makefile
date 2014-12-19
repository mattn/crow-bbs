all : bbs

bbs : bbs.cxx
	g++ -std=gnu++11 -o bbs -I. bbs.cxx -lmysqlcppconn -lboost_system -lpthread
