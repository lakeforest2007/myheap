File: readme.txt
Author: Julie Wang
----------------------

implicit
--------
(1) Describe your design decisions you made for implicit allocator, and why you made those choices.
For the implicit allocator, I used first-fit search to search for the first header that is free to use, because of this, every time `mymalloc` is called the search starts back from the beginning and iteraters linearly through the heap to check each header status. This design choice takes O(n) time and might be expensive when most of the free blocks are towards the very end of the heap. For reallocation of payload data, I decided to use in-place realloc. For this reason, most of the operations are constant and quick.

(2) Summarize its overall performance characteristics - strengths and weaknesses? How did you attempt to optimize your allocator?
For `pattern-mixed`, `mymalloc` averaged about 300 instructions per request, which definitely is not ideal. Looking at the individual line instructions, I found that most of the counts came from the for-loop that iterates through each header and the conditional-if inside of it that checks the header status and size of payload. Likewise, when evaluated on the `trace-chs` that had about 8 times more requested as mixed, almost 90% of the total instruction counts were from `mymalloc` alone. This expense is likely because of the first-fit design choice.
For `myfree`, each request averaged about 10 instructions. This is close to the best-case scenario and the 2nd fastest among the three functions. Bitwise operations were used to optimize calculations. Similarly for `trace-chs`, instruction count, when compared with `mixed`, showed that `myfree` operates constantly.
Lastly, for `myrealloc`, there were about 1,000 instructions total. One of the reasons for this optimization is using in-place reallocation. Without, the count was higher because we need more calls to `mymalloc` (and `mymalloc` is the most expensive out of all three). 

(3) Share an amusing anecdote / insight.
I didn't realize how expensive running `mymalloc` was until I used callgrind to look at the instruction counts. If I had more time, I would love to try next-fit or best-fit and compare their difference in performance. 


explicit
--------
(1) Describe your design decisions you made for implicit allocator, and why you made those choices.
For the explicit allocator, I used a LIFO doubly linked list. To keep this last-in-first-out design choice consistent, every time I free a node, I add it back to the beginning of the list. An advantage of this approach is that it only takes constant time - we do not have to iterate through the entire list each time we add a free node. However, a downside of this choice is that if a node contained a huge sized block (for example, this is usually the last header in the heap) is added to the beginning of the list, everytime we allocate memory we have to split the block. This can increase the expence.
For coalescing measures, I combined blocks to the right for `myrealloc`, and used in-place realloc after coalescing. If there were extra padding that is big enough to store a header and two pointers, I splitted the block and added the extra to the freelist to improve utilization.

(2) Summarize its overall performance characteristics - strengths and weaknesses? How did you attempt to optimize your allocator?
About 40,127 total instructions were collected for the mixed script, in which `mymalloc` contributed to 16,790 of those instructions - we see that this is much closer to the best case scenario than the implicit implementation of it. This is one of the allocator's plus due to the observation that `mymalloc` only iterates through the freed nodes, so the runtime is every so rarely O(n). For `myfree`, about 9,500 insturctions were collected. This number isn't as good as the one for implicit and reasons for this is that we see that the extra instructions are mainly coming from coalescing checks. This indicates that while coalescing is a key factor for increased utilization, it might not be the most optimal in speed. For `myrealloc`, only 2,500 instructions were counted. This takes the least amount of instructions out of all three, although it is still not as good as the count for implicit's myrealloc. This is also due to the coalescing that is involved. On the plus side, most instructions have constant growth.

When evaluated on the trace-chs script, mymalloc's instruction counts was similar to the ratio of mymalloc instructions to toal instructions for the mixed script, even though there were 8 times as many requests to process. And in myfree, coalesce is still the most expensive. Lastly, myrealloc had about 79,000 instructions. Here we see that unlike the counts from the mixed script, the instructions count for myrealloc is actually higher than myfree. This might be because of the in-place reallocation, which necessitates more conditional checks as requests increase. 

(3) Share an amusing anecdote / insight.
To be honest, I didn't think that I would finish the explicit allocator. My program had no bugs when ran with example and pattern script, but bug after bug came up when running trace scripts. After pulling all-nighters, I decided to add more test cases to validate_heap(), and sure enough, running the pattern scripts again outputted errors with my internal heap organization. This taught me that validate_heap() is so extremely important. If I were to do this project again, I would make sure that my validate_heap function is fully implemented and tested right from the start. 


Tell us about your quarter in CS107!
-----------------------------------
While challenging, this class taught me so much, like how important is it to
set milestones and test my program at each step, and the 'behind the scenes' of how
programs are executed. Also, the feedback on style the TAs gave me were all very
targeted and specific, so I was able to implement those new suggestions effectively.
Thank you for making the class fun (most of the time) and rewarding:)



