for numThreads in 1 2 4 8
do
    for numNodes in 5 10 50 100 500 1000 2000 3000
    do
        g++ -D NUMNODES=${numNodes} -D NUMT=${numThreads} main.cpp -o SuperQuad -lm -fopenmp
        ./SuperQuad
    done
done
