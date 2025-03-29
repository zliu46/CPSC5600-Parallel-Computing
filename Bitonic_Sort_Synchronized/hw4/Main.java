/**
 * ParallelBitonicSortMain
 * Entry point for testing Parallel Bitonic Sort.
 *
 * - Runs sorting with `P = {1, 2, 4, 8}.
 * - Measures performance in terms of **arrays sorted per 10 seconds.
 *
 * @author Zhou Liu
 * @version 1.0
 */
public static void main(String[] args) {
    int N = 1 << 22;

    ParallelBitonicSort.setN(N);

    int[] threadCounts = {1, 2, 4, 8};

    for (int P : threadCounts) {
        System.out.println("Starting test with P = " + P);
        long startTime = System.currentTimeMillis();
        int arraysSorted = 0;

        startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < 10_000) {
            ParallelBitonicSort.data = ParallelBitonicSort.generateRandomArray();
            ParallelBitonicSort.parallelBitonicSort(P);
            arraysSorted++;
        }
        // Display performance results
        System.out.printf("Threads: %d | Arrays sorted: %d%n", P, arraysSorted);
    }
}

