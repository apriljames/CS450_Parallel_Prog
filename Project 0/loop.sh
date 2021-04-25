
g++ -DNUMT=1 ArrayMult.cpp -o ArrayMult -lm -fopenmp
./ArrayMult
g++ -DNUMT=4 ArrayMult.cpp -o ArrayMult -lm -fopenmp
./ArrayMult
