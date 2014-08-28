.global foo
//foo: .word 0
.word @foo
//.word @foo + 2
.set foo, 3
//.set foo, 4
.word @foo + 2
.set foo, 5
.word @foo + 2
.set foo, 6
.word @foo + 2
.set foo, -4
.word @foo + 2
//foo: .word 0
//foo: .word 1
