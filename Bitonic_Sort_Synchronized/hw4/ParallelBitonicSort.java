import java.util.Random;
import java.util.concurrent.CyclicBarrier;

/**
 * ParallelBitonicSort
 * A parallelized Bitonic Sort implementation using Java threads and CyclicBarrier.
 *
 * @author Zhou Liu
 * @version 1.0
 */
public class ParallelBitonicSort {
    // Size of dataset
    private static int N;
    // Shared array for sorting
    public static int[] data;
    // Barrier for thread synchronization
    private static CyclicBarrier barrier;

    /**
     * Generates an array of random integers.
     * @return Randomized integer array of given size.
     */
    public static int[] generateRandomArray() {
        Random rand = new Random();
        int[] array = new int[N];
        for (int i = 0; i < N; i++) {
            array[i] = rand.nextInt(1000);
        }
        return array;
    }

    public static void setN(int newN) {
        N = newN;
    }
    /**
     * Performs parallel Bitonic Sort using multiple threads.
     *
     * - Each thread handles a subset of the array.
     * - Threads synchronize via CyclicBarrier after every `j` stage.
     * - Sorting follows the standard Bitonic Merge Sort pattern.
     *
     * @param P Number of threads to use.
     */
    static void parallelBitonicSort(int P) {
        barrier = new CyclicBarrier(P); // Reset barrier for each sorting task
        Thread[] threads = new Thread[P];

        for (int p = 0; p < P; p++) {
            final int threadID = p;
            threads[p] = new Thread(() -> {
                for (int k = 2; k <= N; k *= 2) { // Step size doubling
                    for (int j = k / 2; j > 0; j /= 2) {
                        int start = (threadID * N) / P;
                        int end = ((threadID + 1) * N) / P;

                        bitonicMerge(start, end, j, k);
                        awaitBarrier(); // Sync once per `j`
                    }
                }
            });
            threads[p].start();
        }

        // Ensure all threads finish sorting
        for (int p = 0; p < P; p++) {
            try {
                threads[p].join();
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * Performs the Bitonic Merge operation on a subset of the array.
     *
     * @param start Start index of the section to merge.
     * @param end End index of the section to merge.
     * @param j Current partition size.
     * @param k Step size of the bitonic merge.
     */
    private static void bitonicMerge(int start, int end, int j, int k) {
        for (int i = start; i < end; i += 16) {
            for (int b = 0; b < 16 && (i + b) < end; b++) {
                int ixj = (i + b) ^ j;
                if (ixj > (i + b)) {
                    if (((i + b) & k) == 0 && data[i + b] > data[ixj]) {
                        swap(i + b, ixj);
                    }
                    if (((i + b) & k) != 0 && data[i + b] < data[ixj]) {
                        swap(i + b, ixj);
                    }
                }
            }
        }
    }

    /**
     * Swaps two elements in the array.
     *
     * @param i Index of the first element.
     * @param j Index of the second element.
     */
    private static void swap(int i, int j) {
        int temp = data[i];
        data[i] = data[j];
        data[j] = temp;
    }

    /**
     * Synchronizes threads at the current stage of sorting.
     * - All threads must reach this point before continuing.
     * - Ensures correctness of the sorting process.
     */
    private static void awaitBarrier() {
        try {
            barrier.await();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}

