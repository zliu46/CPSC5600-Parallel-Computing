import java.util.concurrent.SynchronousQueue;
import java.util.Arrays;
/**
 * BitonicPipeline: Implements a pipelined bitonic sorter.
 * - Uses 7 worker threads for sorting stages.
 * - Uses SynchronousQueue for inter-thread communication.
 * - Measures throughput over a fixed time window.
 *
 * @author Zhou Liu
 * @version 1.0
 */
public class BitonicPipeline {
    public static final int N = 1 << 22;  // Power of 2 array size
    public static final int TIME_ALLOWED = 10;  // Execution time in seconds

    public static void main(String[] args) throws InterruptedException {
        // Initialize inter-thread communication queues
        @SuppressWarnings("unchecked")
        SynchronousQueue<double[]>[] stageQueues = (SynchronousQueue<double[]>[]) new SynchronousQueue[7];
        for (int i = 0; i < stageQueues.length; i++) {
            stageQueues[i] = new SynchronousQueue<>();
        }

        // Random Array Generators (Each generates 1/4 of the total array)
        Thread array1 = new Thread(new RandomArrayGenerator(N / 4, stageQueues[0]));
        Thread array2 = new Thread(new RandomArrayGenerator(N / 4, stageQueues[1]));
        Thread array3 = new Thread(new RandomArrayGenerator(N / 4, stageQueues[2]));
        Thread array4 = new Thread(new RandomArrayGenerator(N / 4, stageQueues[3]));

        // StageOne Threads (Sorts initial sections)
        Thread stageOne1 = new Thread(new StageOne(stageQueues[0], stageQueues[4]), "StageOne-1");
        Thread stageOne2 = new Thread(new StageOne(stageQueues[1], stageQueues[5]), "StageOne-2");
        Thread stageOne3 = new Thread(new StageOne(stageQueues[2], stageQueues[4]), "StageOne-3");
        Thread stageOne4 = new Thread(new StageOne(stageQueues[3], stageQueues[5]), "StageOne-4");

        // Bitonic Merge Stages
        Thread bitonicStage1 = new Thread(new BitonicMergeTask(stageQueues[4], stageQueues[5], stageQueues[6]), "BitonicStage-1");
        Thread bitonicStage2 = new Thread(new BitonicMergeTask(stageQueues[6], stageQueues[6], stageQueues[6]), "BitonicStage-2");
        Thread bitonicStage3 = new Thread(new BitonicMergeTask(stageQueues[6], stageQueues[6], stageQueues[6]), "BitonicStage-3");

        // Start All Threads
        array1.start();
        array2.start();
        array3.start();
        array4.start();
        stageOne1.start();
        stageOne2.start();
        stageOne3.start();
        stageOne4.start();
        bitonicStage1.start();
        bitonicStage2.start();
        bitonicStage3.start();

        // Monitor output and measure throughput
        long startTime = System.currentTimeMillis();
        long endTime = startTime + TIME_ALLOWED * 1000;
        int arraysProcessed = 0;

        while (System.currentTimeMillis() < endTime) {
            double[] result = stageQueues[6].poll(10, java.util.concurrent.TimeUnit.SECONDS);
            if (RandomArrayGenerator.isSorted(result)) {
                arraysProcessed++;
            } else if (result == null){
                System.err.println("failed");
            }
        }

        // **Step 6: Print Results**
        System.out.println("Sorted " + arraysProcessed + " arrays (each: " + N + " doubles) in "
                + TIME_ALLOWED + " seconds.");
    }

    /**
     * A Runnable task for bitonic merging.
     * - Merges two sorted arrays into a bitonic sequence.
     * - Performs bitonic sort to produce a fully sorted output.
     */
    private static class BitonicMergeTask implements Runnable {
        private final SynchronousQueue<double[]> input1;
        private final SynchronousQueue<double[]> input2;
        private final SynchronousQueue<double[]> output;

        public BitonicMergeTask(SynchronousQueue<double[]> input1, SynchronousQueue<double[]> input2,
                                SynchronousQueue<double[]> output) {
            this.input1 = input1;
            this.input2 = input2;
            this.output = output;
        }

        @Override
        public void run() {
            try {
                BitonicStage bitonicStage = new BitonicStage();
                while (true) {
                    double[] arr1 = input1.take();
                    double[] arr2 = input2.take();
                    double[] sortedArray = bitonicStage.process(arr1, arr2);
                    output.put(sortedArray);
                }
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
    }
}
