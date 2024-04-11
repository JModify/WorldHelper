#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <csse2310a1.h>
#include <string.h>
#include <ctype.h>

#define LENGTH_ARGUMENT "-len"
#define WITH_ARGUMENT "-with"
#define WITHOUT_ARGUMENT "-without"
#define ALPHA_ARGUMENT "-alpha"
#define BEST_ARGUMENT "-best"

#define LENGTH_MIN_BOUND 4
#define LENGTH_MAX_BOUND 9
#define DEFAULT_LENGTH 5

#define MAX_WORD_LENGTH 50
#define DELETED_PLACEHOLDER "*DELETED*"

/* Pattern structure
 * -----------------
 * Defines the Pattern argument structure.
 * 
 * value - the pattern itself
 * index - the index the pattern is found in the command line.
 */
typedef struct {
    char* value;
    int index;
} Pattern;

// Function Prototypes
int handle_length_argument(int argc, char** argv);
char* handle_alphabetical_argument(int argc, char** argv, 
	char* argumentToHandle);
Pattern handle_pattern_argument(int argc, char** argv, int length);
bool is_sorting_best(int argc, char** argv);
bool is_sorting_alpha(int argc, char** argv);

bool command_line_validity_check(int argc, char** argv);

bool is_string_alpha(char* string);
bool has_duplicates(int array_size, char** array, char* to_find);
bool matches_pattern_format(char* string, int expectedLength);
bool array_contains(int array_size, char** array, char* element);
char* format_word(char* word);

FILE* get_dictionary_file(void);
int print_dictionary(FILE* dictionaryFile, Pattern pattern, int length, 
	char* with, char* without, bool alpha, bool best);

bool word_without_characters(char* word, char* characters);
bool word_with_characters(char* word, char* characters);
bool matches_pattern(char* word, char* pattern);

int sort_word_best(const void* word1, const void* word2);
int sort_word_alpha(const void* word1, const void* word2);
void print_words(char** wordsToPrint, int totalWords);

void remove_duplicates(int arraySize, char** array);

int main(int argc, char** argv) {
    int length = handle_length_argument(argc, argv);

    // Length argument must be checked first since the pattern handler needs
    // it to handle the pattern argument from command line.
    if (length) {
	Pattern pattern = handle_pattern_argument(argc, argv, length);
	char* with = handle_alphabetical_argument(argc, argv, WITH_ARGUMENT);
	char* without = handle_alphabetical_argument(argc, argv,
		WITHOUT_ARGUMENT);
	bool alpha = is_sorting_alpha(argc, argv);
	bool best = is_sorting_best(argc, argv);

	// Executes if there are no duplicates and all arguments starting with
	// '-' character are followed by with, without, len, best or alpha.
	if (command_line_validity_check(argc, argv)) {

	    // Checks argument specific validity and that -best and -alpha
	    // were not both specified.
	    if (with && without && !(best && alpha)) {

		// Executes if the pattern handler was not able to find
		// a valid pattern. 
		if (pattern.value == NULL) {
		    fprintf(stderr, "wordle-helper: pattern must be of length "
			    "%d and only contain underscores and/or letters\n"
			    , length);
		    return 2;
		}

		FILE* dictionaryFile = get_dictionary_file();
		int printedWords = print_dictionary(dictionaryFile, pattern, 
			length, with, without, alpha, best);

		// Executes if there were no words which matches the
		// parameters of the print function and so no words were
		// printed.
		if (!printedWords) {
		    return 4;
		}

		// Successful run
		return 0;
	    }
	}
    }

    fprintf(stderr, "Usage: wordle-helper [-alpha|-best] [-len len] [-with"
	    " letters] [-without letters] [pattern]\n");
    return 1;
}

/* print_dictionary()
 * ------------------
 * Prints the dictionary to standard output using the parameters provided.
 *
 * Parameters:
 * dictionary_file - dictionary file containing a list of words.
 * length - string length a word in the dictionary should be.
 * with - characters a word should contain.
 * without - characters a word must not contain.
 * alpha - whether sorting of words should use an alphabetical algorithm
 * best - whether sorting of words should use the best algorithm
 *
 * Errors:
 * The function will return the value 0 if no words were found
 * to match the arguments passed to this function i.e. no successful words.
 *
 * Returns: the number of successful words printed.
 */
int print_dictionary(FILE* dictionaryFile, Pattern pattern, int length, 
	char* with, char* without, bool alpha, bool best) {

    // List of successful words to be sorted later.
    char** toSort = malloc(sizeof(char*) * 1);

    // Allocate memory for the initial word.
    toSort[0] = malloc(sizeof(char) * MAX_WORD_LENGTH);

    int elementCount = 0;
    char* line = malloc(sizeof(char) * MAX_WORD_LENGTH);
    while (fgets(line, MAX_WORD_LENGTH, dictionaryFile)) {
	char* word = strdup(line);
	// Upper case word, exclude new line and append terminating char.
	char* formattedWord = format_word(word);

	// Executes if the word matches expected length and is alphabetical.
	if ((strlen(formattedWord) == length)
		&& is_string_alpha(formattedWord)) {

	    // Executes if the word contains the specified characters
	    if (word_with_characters(formattedWord, with) 
		    && word_without_characters(formattedWord, without)) {

		// Executes if the word matches the pattern 
		// format provided
		if (matches_pattern(formattedWord, pattern.value)) {
		    toSort[elementCount] = strdup(formattedWord);
		    elementCount++;
		    toSort = realloc(toSort, sizeof(char*) 
			    * (elementCount + 1));
		}
	    }
	}
	// Free the memory allocated to format word properly.
	free(formattedWord);
    }

    if (best) {
	qsort(toSort, elementCount, sizeof(char*), sort_word_best);
	remove_duplicates(elementCount, toSort);
    } else if (alpha) {
	qsort(toSort, elementCount, sizeof(char*), sort_word_alpha);
	remove_duplicates(elementCount, toSort);
    }

    print_words(toSort, elementCount);
    free(toSort);
    fclose(dictionaryFile);
    return elementCount;
}

/* remove_duplicates()
 * -------------------
 * Removes duplicates from an array. Function acomplishes this by simple
 * replacing the duplicate with a placeholder which should be later skipped
 * by the printf function when detected.
 *
 * Parameters:
 * arraySize - size of array
 * array - array to remove duplicates
 */
void remove_duplicates(int arraySize, char** array) {
    for (int i = 0; i < arraySize; i++) {
	char* element = array[i];
	int nextIndex = i + 1;

	if (nextIndex < arraySize) {
	    char* nextElement = array[nextIndex];

	    if (strcmp(nextElement, element) == 0) {
		array[i] = "*DELETED*";
	    }
	}
    }
}

/* sort_word_best()
 * ----------------
 * Sorts word in best order using guess_compare() function.
 * If two words have the same likely hood (result of guess_compare() == 0), 
 * then the sort_word_alpha result is returned.
 *
 * Parameters:
 * word1 - first word to be compared
 * word2 - second word to be compared
 *
 * Reference: man page for guess_compare, man page for qsort
 *
 * Returns: An integer < 0 or > 0 depending on whether word 1 is worse,
 *          or better, if a word is the same, the sort_word_alpha() function
 *          result of these words is returned.
 */
int sort_word_best(const void* word1, const void* word2) {
    int result = guess_compare(*(char* const*) word2, *(char* const*) word1);

    return result != 0 ? result : sort_word_alpha(word1, word2);
}

/* sort_word_alpha()
 * -----------------
 * Sorts word in alphabetical order using the strcmp function.
 *
 * Parameters:
 * word 1 - first word to be compared
 * word 2 - second word to be compared
 *
 * Reference: strcmp man page, qsort man page
 *
 * Returns: An integer < 0, 0 or > 0 if word1 is found, respectively, to be
 *          less than, to match, or be greater than word2.
 */
int sort_word_alpha(const void* word1, const void* word2) {
    return strcmp(*(char* const*) word1, *(char* const*) word2);
}

/* word_without_characters()
 * -----------------------
 * Determine if the word does not contain the given character set.
 * Different to word_with_characters() as this function does not care
 * for duplicates. The moment a character is found in the word which shouldn't
 * appear, this function will return false.
 *
 * Parameters:
 * word - word to check
 * character - character set which should not appear in the word
 *
 * Returns: true if the word does not contains any of the characters in the
 *          provided set, else false.
 */
bool word_without_characters(char* word, char* characters) {
    if (strlen(characters) == 0) {
	return true;
    }

    for (int i = 0; i < strlen(characters); i++) {
	if (strchr(word, toupper(characters[i]))) {
	    return false;
	}
    }

    return true;
}

/* word_with_characters()
 * --------------------
 * Determines if a word contains a series of characters with respect to
 * the number of times a character is repeated in a given character set.
 *
 * Example:
 * If the letter 'a' is listed in the characters parameter twice, this
 * function will only return true if 'a' is also listed twice in
 * the word. 
 *
 * Parameters:
 * word - word to check if it contains specified characters
 * characters - the set of characters to check that the word contains.
 *
 * Returns: true if the word contains the specified characters, else false.
 */
bool word_with_characters(char* word, char* characters) {
    if (strlen(characters) == 0) {
	return true;
    }

    // Start by beginning the word contains the target characters
    bool contains = true;

    for (int i = 0; i < strlen(characters); i++) {
	char characterToCheck = toupper(characters[i]);

	// Break the loop if the word does not contain this character. 
	if (strchr(word, characterToCheck) == NULL) {
	    contains = false;
	    break;
	}

	// Count the number of times this character appears within the
	// target characters parameter itself. Since we know it is contained
	// within itself, the starting value is 1.
	int count = 1;
	for (int j = 0; j < strlen(characters); j++) {
	    if (i != j) {
		if (toupper(characters[j]) == characterToCheck) {
		    count += 1;
		}
	    }
	}

	// Count the number of times this character appears in the word
	int occurencesInWord = 0;
	for (int j = 0; j < strlen(word); j++) {
	    char wordCharacter = toupper(word[j]);

	    if (wordCharacter == characterToCheck) {
		occurencesInWord += 1;
	    }
	}

	// Executes if the number of times this character appears in the word
	// to check is not equal to the number of times this character
	// is written in the target "characters" set.
	if (occurencesInWord < count) {
	    contains = false;
	    break;
	}
    }

    return contains;
}

/* matches_pattern()
 * -----------------
 * Determines if a word matches a particular pattern.
 *
 * Parameter:
 * word - word to attempt match.
 * pattern - pattern word must match with.
 *
 * Returns: true if word matches the pattern, else false.
 */
bool matches_pattern(char* word, char* pattern) {

    // An empty pattern argument is still valid.
    if (strlen(pattern) == 0) {
	return true;
    }

    for (int i = 0; i < strlen(word); i++) {
	if (pattern[i] != '_') {
	    char patternUpper = toupper(pattern[i]);

	    if (word[i] != patternUpper) {
		return false;
	    }
	}
    }

    return true;
}

/* handle_pattern_argument()
 * -------------------------
 * Handles the pattern argument using the arguments parsed to the main
 * function. 
 *
 * Parameters:
 * argc - number of arguments parsed to main function.
 * argv - arguments parsed to main function.
 * length - expected length of the pattern.
 *
 * Error Conditions:
 * NULL is returned if an argument does not start with -, does not follow
 * -with, -len or -without but is of invalid pattern format (contains 
 *  characters other than letters/underscores OR is of unexpected length).
 *
 * Returns:
 *     (NULL) - if the pattern provided is invalid.
 *     (EMPTY_STRING) - if there is no pattern provided - still valid.
 *     (PATTERN STRING) - the validated pattern provided.
 */
Pattern handle_pattern_argument(int argc, char** argv, int length) {
    char* patternValue = "";

    int patternIndex = -1;
    // Finds the first suitable pattern argument.
    for (int i = 1; i < argc; i++) {
	char* argument = argv[i];

	if (*argument != '-') {
	    char* previousArgument = argv[i - 1];

	    // Executes if the argument is not immediately following
	    // the -len -with or -without arguments.
	    if (strcmp(previousArgument, WITH_ARGUMENT)
		    && strcmp(previousArgument, WITHOUT_ARGUMENT)
		    && strcmp(previousArgument, LENGTH_ARGUMENT)) {

		// Executes if the argument contains only alphabetical
		// characters and/or underscores and is of expected length.
		if (matches_pattern_format(argument, length)) {
		    patternValue = argument;
		    patternIndex = i;
		    break;
		} else {
		    patternValue = NULL;
		    break;
		}

	    }


	}

    }

    Pattern pattern;
    pattern.value = patternValue;
    pattern.index = patternIndex;

    return pattern;
}

/* is_sorting_alpha()
 * ------------------
 * Determines if the sorting algorithm is set to alpha i.e. the -alpha
 * flag is present. This function does not conduct any sort of validation
 * checks, this is done in the general_argument_validity_check() function.
 *
 * Parameters:
 * argc - total number of arguments parsed to main function
 * argv - arguments parsed to main function.
 *
 * Returns: true if the sorting algorithm is 'best', else false.
 */
bool is_sorting_alpha(int argc, char** argv) {
    return array_contains(argc, argv, ALPHA_ARGUMENT) ? true : false;
}

/* is_sorting_best()
 * -----------------
 * Determines if the sorting algorithm is set to best i.e. the -best
 * flag is present. This function does not conduct any sort of validation
 * checks, this is done in the general_argument_validity_check() function.
 *
 * Parameters:
 * argc - total number of arguments parsed to main function
 * argv - arguments parsed to main function.
 *
 * Returns: true if the sorting algorithm is 'best', else false.
 */
bool is_sorting_best(int argc, char** argv) {
    return array_contains(argc, argv, BEST_ARGUMENT) ? true : false;
}

/* command_line_validity_check()
 * -----------------------------
 * Detects general argument validity. More specific validity checks are
 * further conducted under an arguments corresponding handler (eg. 
 * handle_length_argument() and/or handle_alpha_argument()).
 *
 * A given argv argument is invalid according to this function if:
 *  - it begins with "-" and is not followed by 
 *    alpha, best, len, with or without.
 *  - it does not begin with -, is not followed by alpha, best, len with or
 *    without AND is a possible duplicated pattern argument (already exists
 *    another argument which does not begin with -).
 *  - the argument starts with -, is followed by alpha, best, len,
 *    with or without but is a duplicated version of one of these.
 *
 * Aditionally, if the number of elements in the main function is greater
 * than 9 (max 9 elements including directory at index 0), false is returned.
 *
 * Parameters:
 * argc - number of arguments parsed to main function.
 * argv - arguments parsed to main function.
 *
 * Returns: true if the general command line is valid according to
 *          specifications above, else false.
 */
bool command_line_validity_check(int argc, char** argv) { 
    if (argc > 9) {
	return false;
    }
    // Executes if the arguments provided contain duplicates of
    // the -best or -alpha commands.
    if (has_duplicates(argc, argv, BEST_ARGUMENT)
	    || has_duplicates(argc, argv, ALPHA_ARGUMENT)
	    || has_duplicates(argc, argv, LENGTH_ARGUMENT)
	    || has_duplicates(argc, argv, WITH_ARGUMENT)
	    || has_duplicates(argc, argv, WITHOUT_ARGUMENT)) {
	return false;
    }

    int possiblePatternArguments = 0;
    for (int i = 1; i < argc; i++) {
	char* argument = argv[i];

	// Executes if the argument begins with the '-' character.
	if (*argument == '-') {
	    // Executes if the argument is not valid (does not match
	    // -len, -with, -without, -best, -alpha).
	    if (strcmp(argument, LENGTH_ARGUMENT)
		    && strcmp(argument, WITH_ARGUMENT)
		    && strcmp(argument, WITHOUT_ARGUMENT)
		    && strcmp(argument, BEST_ARGUMENT)
		    && strcmp(argument, ALPHA_ARGUMENT)) {

		return false;
	    }
	} else {
	    char* previousArgument = argv[i - 1];

	    // Executes if argument is a possible pattern argument
	    // (does not start with '-' and does not follow -len -with or
	    // -without
	    if (strcmp(previousArgument, LENGTH_ARGUMENT)
		    && strcmp(previousArgument, WITHOUT_ARGUMENT)
		    && strcmp(previousArgument, WITH_ARGUMENT)) {

		possiblePatternArguments += 1;
	    }
	}
    }

    // Too many arguments not starting with -
    if (possiblePatternArguments > 1) {
	return false;
    }
    return true;
}

/* handle_alphabetical_argument()
 * ------------------------------
 * Handles an alphabetical argument passed to the main function. Alphabetical
 * arguments must be in the form -ARG (ALPHABETICAL_STRING). This function
 * will look for any arguments in this form which match the argumentToHandle
 * value, if one is found, it will be validated and returned.
 *
 * Intended purpose of this function is to handle both -with and -without.
 *
 * Parameters:
 * argc - total number of arguments passed to main function
 * argv - arguments passed to the main function.
 * argumentToHandle - argument type to handle (-with or -without)
 *
 * Error Conditions:
 * NULL is returned if the argumentToHandle argument is listed in the command
 * line but is not followed by only uppercase / lowercase letters.
 *
 * Returns:
 *    - (NULL) if the argumentToHandle is found to be invalid.
 *    - (Empty String) if the argumentToHandle could not be found.
 *    - (char*) the validated "with" argument.
 */
char* handle_alphabetical_argument(int argc, char** argv,
	char* argumentToHandle) {
    char* with = "";

    for (int i = 0; i < argc; i++) {

	// Executes if this argument is the argumentToHandle
	if (strcmp(argv[i], argumentToHandle) == 0) {
	    int nextIndex = i + 1;

	    // Check that the next index is not out of bounds for the array.
	    if (nextIndex < argc) {
		char* followingArgument = argv[nextIndex];

		// Executes if the following string is alphabetical
		if (is_string_alpha(followingArgument)) {
		    with = followingArgument;
		    break;
		}
	    }

	    // Executes if there were any errors with the following string
	    // making the argumentToHandle invalid. Validity is then
	    // dealt with in the main function.
	    with = NULL;
	    break;
	}
    }

    return with;
}

/* handle_length_argument()
 * ------------------------
 * Handles the wordle length value for the program based on the arguments
 * passed to the main function.
 *
 * Parameters:
 * argc - total number of arguments passed to main function
 * argv - arguments passed to the main function.
 *
 * Error Conditions:
 * An integer value of 0 is returned if the -len argument is specified in the
 * command line but is not followed by a single digit positive integer
 * which is greater or ethan to 4 but less than or equal to 9.
 *
 * Returns: 
 *     - (0) if the given length argument is invalid.
 *     - (5) as a default if the length argument is not provided.
 *     - (4-9 inclusive) if the length argument is provided and valid.
 */
int handle_length_argument(int argc, char** argv) {
    int length = DEFAULT_LENGTH;

    for (int i = 0; i < argc; i++) {
	// Handles possible length argument
	if (strcmp(argv[i], LENGTH_ARGUMENT) == 0) {
	    int nextIndex = i + 1;

	    // Check that there exists a following  argument
	    if (nextIndex < argc) { 
		char* followingArgument = argv[nextIndex];


		// Check that the next argument is a single digit.
		if (strlen(followingArgument) == 1) {

		    // Check that the provided single digit is an integer.
		    if (isdigit(*followingArgument) != 0) {

			int num = atoi(followingArgument);

			// Executes if the length entry is within valid bounds.
			if (num >= LENGTH_MIN_BOUND
				&& num <= LENGTH_MAX_BOUND) {

			    length = num;
			    break;

			}
		    }
		}
	    }

	    // Executes if there were any errors with the following integer.
	    // This value is then used to check validity in the main function.
	    length = 0;
	    break;
	}
    }

    return length;
}

/* get_dictionary_file()
 * ---------------------
 * Retrieves the wordle dictionary.
 *
 * In the case where the environment variable "WORDLE_DICTIONARY" is set, then
 * this function will try and open the directory provided there. If the
 * value of this variable is not a valid directory that can be opened, then
 * this function will exit the program with exit code 3.
 *
 * In the case where the above environment variable is not set, then this
 * function will use the default directory "/usr/share/dict/words" as
 * the dictionary file.
 *
 * Error Conditions:
 * Function exits program with exit code 3 if environment variable is set,
 * but value specified cannot be opened via fopen().
 *
 * Reference: getenv man page, fopen man page.
 *
 * Returns: the dictionary file containing a list of words.
 */
FILE* get_dictionary_file(void) {
    char* environmentVariable = getenv("WORDLE_DICTIONARY");

    FILE* dictionary;
    if (environmentVariable) {
	dictionary = fopen(environmentVariable, "r");

	// Executes if there exists an environment variable, but the directory
	// value for this variable could not be opened.
	if (!dictionary) {
	    fprintf(stderr, "wordle-helper: dictionary file \"%s\" "
		    "cannot be opened\n", environmentVariable);
	    exit(3);
	}

    } else {
	dictionary = fopen("/usr/share/dict/words", "r");
    }

    return dictionary;
}

/* array_contains()
 * ----------------
 * Determines whether a given array of strings contains an element.
 *
 * Parameters:
 * array_size - size of array.
 * array - array to check for element
 * element - element to check if it is contained in the array.
 *
 * Returns: true if array contains element, else false.
 */
bool array_contains(int array_size, char** array, char* element) {
    for (int i = 0; i < array_size; i++) {
	char* entry = array[i];

	if (strcmp(entry, element) == 0) {
	    return true;
	}

    }

    return false;
}

/* has_duplicate()
 * --------------
 * Determines whether the given array of strings contains duplicates of the 
 * string element it is checking against. 
 *
 * Parameters:
 * array_size - size of array to check for duplicates
 * array - array which is being checked for duplicates
 * element - string element being checked for duplicate entries of.
 *
 * Returns: true if duplicate entries of this element are found, else false.
 */
bool has_duplicates(int array_size, char** array, char* element) {
    int count = 0;
    for (int i = 0; i < array_size; i++) {
	if (strcmp(array[i], element) == 0) {
	    count += 1;
	}
    }

    return count < 2 ? false : true;
}

/* is_string_alpha()
 * -----------------
 * Determines whether or not a given string is alphabetical.
 * This function will go through each character in the string (char*) 
 * and run the isalpha function to check. If any one of these characters
 * fails to parse the isalpha check, the function will return false.
 *
 * Special case:
 * If the string provided is empty, the function will consider the string
 * non-alphabetical and so a value of false (0) will be returned.
 *
 * Parameters:
 * string - string being checked that it is alphabetical.
 *
 * Returns: true if all characters in the string are alphabetical, else false.
 */
bool is_string_alpha(char* string) {
    int stringLength = strlen(string);

    // Special case - string is empty so cannot be alphabetical
    if (stringLength == 0) {
	return false;
    }

    for (int i = 0; i < stringLength; i++) {
	if (!isalpha(string[i]) && string[i] != '\n') {
	    return false;
	}
    }

    return true;
}

/* format_word()
 * -------------
 * Formats a dictionary word to upper case, excludes the new line character
 * and appends a null terminator - making the word an upper case string.
 *
 * Parameters:
 * word - dictionary word to format as upper case string.
 *
 * Note: Memory should be free()'d when done with formatted string.
 *
 * Returns: String in upper case which excludes the new line character.
 */
char* format_word(char* word) {
    int wordLength = strlen(word);
    char* duplication = malloc(sizeof(char) * wordLength);

    for (int i = 0; word[i] != '\n'; i++) {
	char upper = toupper(word[i]);
	duplication[i] = upper;
    }

    duplication[wordLength - 1] = '\0';
    return duplication;
}

/* matches_pattern_format()
 * ------------------------
 * Determines whether a given string matches the pattern format.
 * The pattern format is defined a string which contains only letters
 * (upper case and lower case) OR underscores - and is of the length
 * provided in the functions parameters.
 *
 * Parameters:
 * string - the string to match with pattern
 * expectedLength - length the pattern is expected to be.
 *
 * Reference: isalpha man page
 *
 * Returns: true if pattern matches the string, else false.
 */
bool matches_pattern_format(char* string, int expectedLength) {
    int stringLength = strlen(string);

    if (stringLength != expectedLength) {
	return false;
    }

    for (int i = 0; i < stringLength; i++) {
	char character = string[i];

	if (!isalpha(character) && character != '_') {
	    return false;
	}
    }

    return true;
}

/* print_words()
 * --------------
 * Prints an array of words to standard output.
 *
 * Intended use is for printing sorted/unsorted dictionary words as this 
 * function will also free each word from memory.
 * This function will also detect if a word has been flagged as duplicated
 * and hence been deleted. In this case, that word will not be printed.
 *
 * Parameters:
 * wordsToPrint - the array of strings to be printed.
 * totalWords - the total number of words which should be printed.
 */
void print_words(char** wordsToPrint, int totalWords) {
    for (int i = 0; i < totalWords; i++) {
	char* word = wordsToPrint[i];

	if (strcmp(word, DELETED_PLACEHOLDER) == 0) {
	    continue;
	}

	fprintf(stdout, "%s\n", word);
	free(word);
    }
}
