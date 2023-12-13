all: serverM.cpp serverS.cpp serverL.cpp serverH.cpp client.cpp
	g++ -o serverM serverM.cpp
	g++ -o serverS serverS.cpp
	g++ -o serverL serverL.cpp
	g++ -o serverH serverH.cpp
	g++ -o client client.cpp

serverM: 
	./serverM

serverS: 
	./serverS

serverL: 
	./serverL

serverH: 
	./serverH

# .PHONY: serverM serverS serverL serverH

clean: 
	$(RM) serverM
	$(RM) serverS
	$(RM) serverL
	$(RM) serverH
	$(RM) client