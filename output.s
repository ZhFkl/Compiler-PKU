decl @getint(): i32

decl @getch(): i32

decl @getarray(*i32): i32

decl @putint(i32)

decl @putch(i32)

decl @putarray(i32, *i32)

decl @starttime()

decl @stoptime()



global @garr = alloc [i32, 10], {6, 7, 8, 9, 10, 11, 12, 13, 14, 15}

fun @main(): i32 {
%entry:
  @arr_0 = alloc [i32, 10]
  @i_11 = alloc i32
  @sum_12 = alloc i32
  %1 = getelemptr @arr_0, 0
  store 1, %1
  %2 = getelemptr @arr_0, 1
  store 2, %2
  %3 = getelemptr @arr_0, 2
  store 3, %3
  %4 = getelemptr @arr_0, 3
  store 4, %4
  %5 = getelemptr @arr_0, 4
  store 5, %5
  %6 = getelemptr @arr_0, 5
  store 0, %6
  %7 = getelemptr @arr_0, 6
  store 0, %7
  %8 = getelemptr @arr_0, 7
  store 0, %8
  %9 = getelemptr @arr_0, 8
  store 0, %9
  %10 = getelemptr @arr_0, 9
  store 0, %10
  store 0, @i_11
  store 0, @sum_12
  jump %while_entry_13
%while_entry_13:
  %14 = load @i_11
  %15 = lt %14, 10
  br %15, %while_body_13, %while_end_13
%while_body_13:
  %16 = load @sum_12
  %17 = add %16, 0
  %18 = add %17, 0
  store %18, @sum_12
  %19 = load @i_11
  %20 = add %19, 1
  store %20, @i_11
  jump %while_entry_13
%while_end_13:
  %21 = load @sum_12
  ret %21


}


