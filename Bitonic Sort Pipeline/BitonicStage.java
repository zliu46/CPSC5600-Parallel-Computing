import java.util.Arrays;
/**
 * BitonicStage: Merges two sorted arrays using bitonic sorting.
 * - Assumes both input arrays are sorted in ascending order.
 * - Reverses the second array to create a bitonic sequence.
 * - Applies bitonic merge to produce a fully sorted array.
 *
 * @author Zhou Liu
 * @version 1.0
 */
public class BitonicStage {

    /**
     * Processes two sorted input arrays into a final sorted array using bitonic merging.
     * @param arr1 First sorted array (ascending).
     * @param arr2 Second sorted array (ascending, will be reversed).
     * @return Fully sorted merged array.
     */
    public double[] process(double[] arr1, double[] arr2) {
        if (arr1 == null || arr2 == null) {
            return new double[0];  // Handle null cases gracefully
        }

        // Reverse the second array to create a valid bitonic sequence
        reverseArray(arr2);

        // Merge the two sequences into one array
        double[] mergedArray = new double[arr1.length + arr2.length];
        System.arraycopy(arr1, 0, mergedArray, 0, arr1.length);
        System.arraycopy(arr2, 0, mergedArray, arr1.length, arr2.length);

        // Apply bitonic merge
        bitonicMerge(mergedArray, 0, mergedArray.length, true);

        return mergedArray;
    }

    /**
     * Reverses an array in-place.
     * @param array The array to reverse.
     */
    private void reverseArray(double[] array) {
        for (int i = 0; i < array.length / 2; i++) {
            double temp = array[i];
            array[i] = array[array.length - 1 - i];
            array[array.length - 1 - i] = temp;
        }
    }

    /**
     * Performs the bitonic merge operation.
     * @param array The array containing a bitonic sequence.
     * @param low Start index.
     * @param count Number of elements to sort.
     * @param ascending If true, sorts in ascending order.
     */
    private void bitonicMerge(double[] array, int low, int count, boolean ascending) {
        if (count > 1) {
            int k = count / 2;
            for (int i = low; i < low + k; i++) {
                if ((array[i] > array[i + k]) == ascending) {
                    swap(array, i, i + k);
                }
            }
            bitonicMerge(array, low, k, ascending);
            bitonicMerge(array, low + k, k, ascending);
        }
    }

    /**
     * Swaps two elements in an array.
     * @param array The array containing the elements.
     * @param i First index.
     * @param j Second index.
     */
    private void swap(double[] array, int i, int j) {
        double temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}
