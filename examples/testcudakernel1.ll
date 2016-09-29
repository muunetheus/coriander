; ModuleID = 'examples/testcudakernel1.cu'
target datalayout = "e-i64:64-v16:16-v32:32-n16:32:64"
target triple = "nvptx64-nvidia-cuda"

@.str = private unnamed_addr constant [5 x i8] c"NONE\00", align 1
@llvm.used = appending global [1 x i8*] [i8* bitcast (i32 ()* @_ZL21__nvvm_reflect_anchorv to i8*)], section "llvm.metadata"

; Function Attrs: nounwind readnone
define internal i32 @_ZL21__nvvm_reflect_anchorv() #0 {
  %1 = tail call i32 @__nvvm_reflect(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str, i64 0, i64 0)) #4
  ret i32 %1
}

; Function Attrs: nounwind readnone
declare i32 @__nvvm_reflect(i8*) #0

; Function Attrs: norecurse nounwind readnone
define float @_Z3barff(float %a, float %b) #1 {
  %1 = fadd float %a, %b
  ret float %1
}

; Function Attrs: norecurse nounwind
define void @_Z7incrvalPf(float* nocapture %a) #2 {
  %1 = load float, float* %a, align 4, !tbaa !17
  %2 = fadd float %1, 3.000000e+00
  store float %2, float* %a, align 4, !tbaa !17
  ret void
}

; Function Attrs: norecurse nounwind
define void @_Z11somekernel1Pf(float* nocapture %a) #2 {
  %1 = getelementptr inbounds float, float* %a, i64 1
  %2 = bitcast float* %1 to i32*
  %3 = load i32, i32* %2, align 4, !tbaa !17
  %4 = bitcast float* %a to i32*
  store i32 %3, i32* %4, align 4, !tbaa !17
  ret void
}

; Function Attrs: norecurse nounwind
define void @_Z3fooPf(float* nocapture %data) #2 {
  store float 1.230000e+02, float* %data, align 4, !tbaa !17
  ret void
}

; Function Attrs: norecurse nounwind
define void @_Z7use_tidPf(float* nocapture %data) #2 {
  %1 = tail call i32 @llvm.ptx.read.tid.x() #6
  %2 = sext i32 %1 to i64
  %3 = getelementptr inbounds float, float* %data, i64 %2
  store float 1.230000e+02, float* %3, align 4, !tbaa !17
  ret void
}

; Function Attrs: norecurse nounwind
define void @_Z11use_blockidPf(float* nocapture %data) #2 {
  %1 = tail call i32 @llvm.ptx.read.ctaid.x() #6
  %2 = sext i32 %1 to i64
  %3 = getelementptr inbounds float, float* %data, i64 %2
  store float 1.230000e+02, float* %3, align 4, !tbaa !17
  ret void
}

; Function Attrs: norecurse nounwind
define void @_Z11use_griddimPf(float* nocapture %data) #2 {
  %1 = tail call i32 @llvm.ptx.read.nctaid.x() #6
  %2 = sext i32 %1 to i64
  %3 = getelementptr inbounds float, float* %data, i64 %2
  store float 1.230000e+02, float* %3, align 4, !tbaa !17
  ret void
}

; Function Attrs: norecurse nounwind
define void @_Z12use_blockdimPf(float* nocapture %data) #2 {
  %1 = tail call i32 @llvm.ptx.read.ntid.x() #6
  %2 = sext i32 %1 to i64
  %3 = getelementptr inbounds float, float* %data, i64 %2
  store float 1.230000e+02, float* %3, align 4, !tbaa !17
  ret void
}

; Function Attrs: norecurse nounwind
define void @_Z13use_template1PfPi(float* nocapture %data, i32* nocapture %intdata) #2 {
  %1 = getelementptr inbounds float, float* %data, i64 1
  %2 = load float, float* %1, align 4, !tbaa !17
  %3 = getelementptr inbounds float, float* %data, i64 2
  %4 = load float, float* %3, align 4, !tbaa !17
  %5 = fadd float %2, %4
  store float %5, float* %data, align 4, !tbaa !17
  %6 = getelementptr inbounds i32, i32* %intdata, i64 1
  %7 = load i32, i32* %6, align 4, !tbaa !21
  %8 = getelementptr inbounds i32, i32* %intdata, i64 2
  %9 = load i32, i32* %8, align 4, !tbaa !21
  %10 = add nsw i32 %9, %7
  store i32 %10, i32* %intdata, align 4, !tbaa !21
  ret void
}

define void @_Z13someops_floatPf(float* nocapture %data) #3 {
  %1 = getelementptr inbounds float, float* %data, i64 1
  %2 = load float, float* %1, align 4, !tbaa !17
  %3 = getelementptr inbounds float, float* %data, i64 2
  %4 = load float, float* %3, align 4, !tbaa !17
  %5 = fsub float %2, %4
  %6 = fdiv float %2, %4
  %7 = fadd float %5, %6
  %8 = fmul float %4, %2
  %9 = fadd float %7, %8
  store float %9, float* %data, align 4, !tbaa !17
  %10 = tail call float @_Z15our_pretend_logf(float %2)
  %11 = load float, float* %data, align 4, !tbaa !17
  %12 = fadd float %10, %11
  store float %12, float* %data, align 4, !tbaa !17
  %13 = load float, float* %1, align 4, !tbaa !17
  %14 = tail call float @_Z15our_pretend_expf(float %13)
  %15 = load float, float* %data, align 4, !tbaa !17
  %16 = fadd float %14, %15
  store float %16, float* %data, align 4, !tbaa !17
  %17 = load float, float* %1, align 4, !tbaa !17
  %18 = tail call float @_Z16our_pretend_tanhf(float %17)
  %19 = load float, float* %data, align 4, !tbaa !17
  %20 = fadd float %18, %19
  %21 = load float, float* %1, align 4, !tbaa !17
  %22 = fpext float %21 to double
  %23 = tail call double @llvm.nvvm.sqrt.rn.d(double %22) #6
  %24 = fpext float %20 to double
  %25 = fsub double %24, %23
  %26 = fptrunc double %25 to float
  store float %26, float* %data, align 4, !tbaa !17
  ret void
}

declare float @_Z15our_pretend_logf(float) #3

declare float @_Z15our_pretend_expf(float) #3

declare float @_Z16our_pretend_tanhf(float) #3

; Function Attrs: norecurse nounwind
define void @_Z11someops_intPi(i32* nocapture %data) #2 {
  %1 = getelementptr inbounds i32, i32* %data, i64 1
  %2 = load i32, i32* %1, align 4, !tbaa !21
  %3 = getelementptr inbounds i32, i32* %data, i64 2
  %4 = load i32, i32* %3, align 4, !tbaa !21
  %5 = sdiv i32 %2, %4
  %6 = add i32 %2, %5
  %7 = add i32 %6, %2
  %8 = mul nsw i32 %4, %2
  %9 = add nsw i32 %8, %7
  store i32 %9, i32* %data, align 4, !tbaa !21
  ret void
}

; Function Attrs: norecurse nounwind
define void @_Z14testbooleanopsPi(i32* nocapture %data) #2 {
  %1 = load i32, i32* %data, align 4, !tbaa !21
  %2 = icmp sgt i32 %1, 0
  %3 = getelementptr inbounds i32, i32* %data, i64 1
  %4 = load i32, i32* %3, align 4, !tbaa !21
  %5 = icmp slt i32 %4, 0
  %6 = and i1 %2, %5
  %7 = zext i1 %6 to i32
  %8 = getelementptr inbounds i32, i32* %data, i64 2
  store i32 %7, i32* %8, align 4, !tbaa !21
  %9 = or i1 %2, %5
  %10 = zext i1 %9 to i32
  %11 = getelementptr inbounds i32, i32* %data, i64 3
  store i32 %10, i32* %11, align 4, !tbaa !21
  ret void
}

; Function Attrs: norecurse nounwind
define void @_Z15testsyncthreadsPf(float* nocapture %data) #2 {
  %1 = tail call i32 @llvm.ptx.read.tid.x() #6
  %2 = sext i32 %1 to i64
  %3 = getelementptr inbounds float, float* %data, i64 %2
  %4 = load float, float* %3, align 4, !tbaa !17
  %5 = fmul float %4, 2.000000e+00
  store float %5, float* %3, align 4, !tbaa !17
  tail call void @llvm.cuda.syncthreads() #6
  %6 = add nsw i32 %1, 1
  %7 = sext i32 %6 to i64
  %8 = getelementptr inbounds float, float* %data, i64 %7
  %9 = load float, float* %8, align 4, !tbaa !17
  %10 = fadd float %9, 2.000000e+00
  store float %10, float* %8, align 4, !tbaa !17
  ret void
}

; Function Attrs: nounwind readnone
declare i32 @llvm.ptx.read.tid.x() #4

; Function Attrs: nounwind readnone
declare i32 @llvm.ptx.read.ctaid.x() #4

; Function Attrs: nounwind readnone
declare i32 @llvm.ptx.read.nctaid.x() #4

; Function Attrs: nounwind readnone
declare i32 @llvm.ptx.read.ntid.x() #4

; Function Attrs: noduplicate nounwind
declare void @llvm.cuda.syncthreads() #5

; Function Attrs: nounwind readnone
declare double @llvm.nvvm.sqrt.rn.d(double) #4

attributes #0 = { nounwind readnone "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="sm_20" "target-features"="+ptx42" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { norecurse nounwind readnone "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="sm_20" "target-features"="+ptx42" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { norecurse nounwind "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="sm_20" "target-features"="+ptx42" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="sm_20" "target-features"="+ptx42" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind readnone }
attributes #5 = { noduplicate nounwind }
attributes #6 = { nounwind }

!nvvm.annotations = !{!0, !1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !11, !13, !13, !13, !13, !14, !14, !13}
!llvm.ident = !{!15}
!nvvm.internalize.after.link = !{}
!nvvmir.version = !{!16}

!0 = !{void (float*)* @_Z11somekernel1Pf, !"kernel", i32 1}
!1 = !{void (float*)* @_Z3fooPf, !"kernel", i32 1}
!2 = !{void (float*)* @_Z7use_tidPf, !"kernel", i32 1}
!3 = !{void (float*)* @_Z11use_blockidPf, !"kernel", i32 1}
!4 = !{void (float*)* @_Z11use_griddimPf, !"kernel", i32 1}
!5 = !{void (float*)* @_Z12use_blockdimPf, !"kernel", i32 1}
!6 = !{void (float*, i32*)* @_Z13use_template1PfPi, !"kernel", i32 1}
!7 = !{void (float*)* @_Z13someops_floatPf, !"kernel", i32 1}
!8 = !{void (i32*)* @_Z11someops_intPi, !"kernel", i32 1}
!9 = !{void (i32*)* @_Z14testbooleanopsPi, !"kernel", i32 1}
!10 = !{void (float*)* @_Z15testsyncthreadsPf, !"kernel", i32 1}
!11 = !{null, !"align", i32 8}
!12 = !{null, !"align", i32 8, !"align", i32 65544, !"align", i32 131080}
!13 = !{null, !"align", i32 16}
!14 = !{null, !"align", i32 16, !"align", i32 65552, !"align", i32 131088}
!15 = !{!"clang version 3.8.0-2ubuntu4 (tags/RELEASE_380/final)"}
!16 = !{i32 1, i32 2}
!17 = !{!18, !18, i64 0}
!18 = !{!"float", !19, i64 0}
!19 = !{!"omnipotent char", !20, i64 0}
!20 = !{!"Simple C/C++ TBAA"}
!21 = !{!22, !22, i64 0}
!22 = !{!"int", !19, i64 0}
