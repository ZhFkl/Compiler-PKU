decl @getint(): i32

decl @getch(): i32

decl @getarray(*i32): i32

decl @putint(i32)

decl @putch(i32)

decl @putarray(i32, *i32)

decl @starttime()

decl @stoptime()



fun @main(): i32 {
%entry:
  @a_0 = alloc [i32,10]
  @b_2 = alloc [i32,10]
  @c_3 = alloc i32
  @d_4 = alloc [i32,0]
  ret 0


}


