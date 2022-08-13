# Lab 1, Part 1

> [Part 1]: Study the generated lexer in file c.lex.cpp, and write a short report (1-5 pages) on the logic of the code produced through the lexical analyser generator. In particular, we are interested in the logic of the function represented by YY_DECL (representing the yylex() function). Keep your report as brief as possible, but do not miss out any interesting fact about the generated code or logic. Your report should reflect your understanding of the logic of the generated lexical analyzer.

## Background

For each token class in our language we define a _regular expression_ that matches all the strings for that token and an _action_ that is called when a token is found in the stream of input chars.

The flex tool reads the regular expressions in a `.l` file and creates a DFA which recognizes all the token classes.

The input character stream is run on the DFA until a `dead state`(A state from which no accepting state can be reached) is reached. Then we backtrack to find an accepting state.  
At this point we have found the _longest prefix_ of the input that matches with a token.

The longest prefix that is found may match to more than one regular expressions. In this case we output the token corresponding to the regex specified first in the file and call the specified action.

## Understanding the code

The code inside the label `yy_match`` runs the DFA.

The array `yy_ec` maps each char to a character class.

Instead of storing a `2-D transition table`, the transition store is stored in a `1-D` array `yy_nxt`.  
To the get `NextState(state, char)` we index the array using `yy_base[state] + yy_ec[char]`.

Actions for accepting states are identified using ids, which are stored in `yy_accept`.  
If a state is non-accepting then `yy_accept[state] = 0`.

While running the `DFA` we may reach into some `accepting` states. We do not stop there and keep recording the `last_accepting_state` and `last_accepting_char_pos` as we move on. This is because we want to find the longest prefix that matches with a token.

The `DFA` is run until we reach the `base state = 699`.  
After we are out of the DFA loop (`yy_match`), we go into the `yy_find_action` label.

The `base state 699` seems to occur in 2 cases:

1. When we have reached a dead state in the DFA i.e. there is no possibility of the current prefix matching with any token. (For ex: a whitespace after "int" `int `).
2. When we have found an accepting state an there is no possibility of a longer prefix being an accepted state. For ex: when we are not inside a quotes and have found a semi-colon`;`.

In the Case[1], `yy_accept[current_state] = 0` and we "roll back" to the last found accepted state and get the corresponding action. In C language, most of the time this is just the previous character.

In Case[2], `yy_accept[current_state] != 0`(which means we are in an accepting state) and we directly get the action for the current state.

Finally we perform the action in `do_action` label using a `switch-case` block.

TODO: Buffer logic.

TODO: EOL(End of line), EOD(??), EOB(end of buffer), EOF(End of file) logic.
