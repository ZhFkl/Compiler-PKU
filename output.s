decl @getint(): i32

decl @getch(): i32

decl @getarray(*i32): i32

decl @putint(i32)

decl @putch(i32)

decl @putarray(i32, *i32)

decl @starttime()

decl @stoptime()



global @buf = alloc [[i32, 100], 2], zeroinit

fun @merge_sort(%l: i32, %r: i32) {
%entry:
  %l_local_0 = alloc i32
  %r_local_1 = alloc i32
  @mid_7 = alloc i32
  @i_16 = alloc i32
  @j_18 = alloc i32
  @k_20 = alloc i32
  @and_tmp_23 = alloc i32
  store %l, %l_local_0
  store %r, %r_local_1
  %2 = load %l_local_0
  %3 = add %2, 1
  %4 = load %r_local_1
  %5 = ge %3, %4
  br %5, %then_6, %else_6
%then_6:
  ret 0
%else_6:
  jump %end_6
%end_6:
  %8 = load %l_local_0
  %9 = load %r_local_1
  %10 = add %8, %9
  %11 = div %10, 2
  store %11, @mid_7
  %12 = load %l_local_0
  %13 = load @mid_7
  call @merge_sort(%12, %13)
  %14 = load @mid_7
  %15 = load %r_local_1
  call @merge_sort(%14, %15)
  %17 = load %l_local_0
  store %17, @i_16
  %19 = load @mid_7
  store %19, @j_18
  %21 = load %l_local_0
  store %21, @k_20
  jump %while_entry_22
%while_entry_22:
  %24 = load @i_16
  %25 = load @mid_7
  %26 = lt %24, %25
  %27 = ne %26, 0
  br %27, %and_right_28, %and_false_28
%and_right_28:
  %29 = load @j_18
  %30 = load %r_local_1
  %31 = lt %29, %30
  %32 = ne %31, 0
  store %32, @and_tmp_23
  jump %and_end_28
%and_false_28:
  store 0, @and_tmp_23
  jump %and_end_28
%and_end_28:
  %33 = load @and_tmp_23
  br %33, %while_body_22, %while_end_22
%while_body_22:
  %34 = getelemptr @buf, 0
  %35 = load @i_16
  %36 = getelemptr %34, %35
  %37 = load %36
  %38 = getelemptr @buf, 0
  %39 = load @j_18
  %40 = getelemptr %38, %39
  %41 = load %40
  %42 = lt %37, %41
  br %42, %then_43, %else_43
%then_43:
  %44 = getelemptr @buf, 1
  %45 = load @k_20
  %46 = getelemptr %44, %45
  %47 = getelemptr @buf, 0
  %48 = load @i_16
  %49 = getelemptr %47, %48
  %50 = load %49
  store %50, %46
  %51 = load @i_16
  %52 = add %51, 1
  store %52, @i_16
  jump %end_43
%else_43:
  %53 = getelemptr @buf, 1
  %54 = load @k_20
  %55 = getelemptr %53, %54
  %56 = getelemptr @buf, 0
  %57 = load @j_18
  %58 = getelemptr %56, %57
  %59 = load %58
  store %59, %55
  %60 = load @j_18
  %61 = add %60, 1
  store %61, @j_18
  jump %end_43
%end_43:
  %62 = load @k_20
  %63 = add %62, 1
  store %63, @k_20
  jump %while_entry_22
%while_end_22:
  jump %while_entry_64
%while_entry_64:
  %65 = load @i_16
  %66 = load @mid_7
  %67 = lt %65, %66
  br %67, %while_body_64, %while_end_64
%while_body_64:
  %68 = getelemptr @buf, 1
  %69 = load @k_20
  %70 = getelemptr %68, %69
  %71 = getelemptr @buf, 0
  %72 = load @i_16
  %73 = getelemptr %71, %72
  %74 = load %73
  store %74, %70
  %75 = load @i_16
  %76 = add %75, 1
  store %76, @i_16
  %77 = load @k_20
  %78 = add %77, 1
  store %78, @k_20
  jump %while_entry_64
%while_end_64:
  jump %while_entry_79
%while_entry_79:
  %80 = load @j_18
  %81 = load %r_local_1
  %82 = lt %80, %81
  br %82, %while_body_79, %while_end_79
%while_body_79:
  %83 = getelemptr @buf, 1
  %84 = load @k_20
  %85 = getelemptr %83, %84
  %86 = getelemptr @buf, 0
  %87 = load @j_18
  %88 = getelemptr %86, %87
  %89 = load %88
  store %89, %85
  %90 = load @j_18
  %91 = add %90, 1
  store %91, @j_18
  %92 = load @k_20
  %93 = add %92, 1
  store %93, @k_20
  jump %while_entry_79
%while_end_79:
  jump %while_entry_94
%while_entry_94:
  %95 = load %l_local_0
  %96 = load %r_local_1
  %97 = lt %95, %96
  br %97, %while_body_94, %while_end_94
%while_body_94:
  %98 = getelemptr @buf, 0
  %99 = load %l_local_0
  %100 = getelemptr %98, %99
  %101 = getelemptr @buf, 1
  %102 = load %l_local_0
  %103 = getelemptr %101, %102
  %104 = load %103
  store %104, %100
  %105 = load %l_local_0
  %106 = add %105, 1
  store %106, %l_local_0
  jump %while_entry_94
%while_end_94:
  ret 


}


fun @main(): i32 {
%entry:
  @n_0 = alloc i32
  %1 = getelemptr @buf, 0
  %2 = getelemptr %1, 0
  %3 = call @getarray(%2)
  store %3, @n_0
  %4 = load @n_0
  call @merge_sort(0, %4)
  %5 = load @n_0
  %6 = getelemptr @buf, 0
  %7 = getelemptr %6, 0
  call @putarray(%5, %7)
  ret 0


}


