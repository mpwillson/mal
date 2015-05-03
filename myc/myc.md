Mal Implementation 
==================

1. When implementing minus and divide, I reused an operand var as the
result.  This lead to a very hard-to-trackdown bug, which manifested
itself in step 4 fib function. The fib argument became less than zero,
which caused mal to seg fault.

2. When adding TCO, I noticed that the do_form code used eval,
whereas the guide stated that eval_ast should be called.  I'd
implemented do by interating of the do form elements and calling
eval on each one, only returning the value of the last element.
Replacing eval with eval_ast in this approach did not work.  The
solution was to call eval_ast with the whole do form (less the do
of course) and then pick out the last value in the returned list.

3. Related to the above, when I turned the above approach into
TCO, I truncated the do form before the last element and passed
this list to eval_ast, returning the last element as the result.
However, like (1), I modified the list in-situ, which meant that
subsequent evaluations steadily trucated the list.  The solution
was to make a copy of the list and truncate that.

4. The seg fault in (1) occured in malloc, which is mostly caused by
user abuse.  It turned out I was writing to an unitialised pointer in
read_atom (reader.c).  Now mal dies with an "out of memory" error
message. I tracked this down in testing mal on FreeBSD, where mal died
immediately, on the defintion of the not function. Nice early failure mode!

5. Comment handling in step 6 caused me a lot of grief.  I wanted to
pass comments back up from the lexer, rather than completely ignoring
them, but had to change read_list slightly so that comment vars were
never added to the list under construction. The other issue concerned
the situation where a comment was the final line of a program file and
did not have trailing newline.  In the end I amended the definition of
load-file to place an extra newline after the slurp to protect the
closing parenthesis of the do form.

6. Step 7 (quoting) took me an age.  It wasn't until I wrote
supporting functions for access to list elements (first and second)
and realised that handle_quasiquote should take a VAR as the argument,
not a list element, that I got my head around it.
