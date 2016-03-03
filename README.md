The following is some clarification on Part A of the project.

While processing the words from the input, the main thread saves the information
in a linked list that looks like below. There are nodes of type `word_item` and
`lines_item` saving the word and the line number respectively. Each `word_item` 
stores the word and a linked list of line numbers. The main thread inserts line
numbers in sorted order, but words are inserted in the beginning of the linked
list. There is one list per worker thread.


	-------------------				-------------------
	|      "word1"    |				|      "word2"    |
	-------------------				-------------------
	|    •   |   •----|----------->	|    •   |   •----|----- .  .    .
	-------------------				-------------------
	     |				     			|
	     |				     			|
	     |				     			|
	     ˅					     		˅
	------------					------------
	| line_nr1 |					| line_nr1 |
	------------					------------
	|    •     |					|    •     |
	------------					------------
	     |				     			|
	     |				     			|
	     |				     			|
	     ˅								 .
	------------
	| line_nr2 |			      	 	.
	------------
	|    •     |
	------------			     	  	.
	     |
	     |
	     |
	     .

	     .


	     .
