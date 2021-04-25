for numThreads in 1 2 4 8
do
    for numTries in 10 100 1000 5000 10000 50000 100000
    do
        g++ -D NUMTRIALS=${numTries} -D NUMT=${numThreads} MonteCarlo.cpp -o MonteCarlo -lm -fopenmp
        ./MonteCarlo
    done
done

