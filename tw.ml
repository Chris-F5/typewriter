(* 595x842 *)

let eat_page =
  let rec parse y =
    try
      let line = read_line () in
        Pdfops.Op_Tm
         (Pdftransform.matrix_of_transform
           [Pdftransform.Translate (20.0, y)]) ::
        Pdfops.Op_Tj line ::
        parse (y -. 12.0)
    with End_of_file -> [] in
  let ops =
    [Pdfops.Op_BT; Pdfops.Op_Tf ("/F0", 12.0)] @
    parse (842.0 -. 32.0) @
    [Pdfops.Op_ET] in
  { (Pdfpage.blankpage Pdfpaper.a4) with Pdfpage.content = [Pdfops.stream_of_ops ops] } in
let page = eat_page in
let pdf, pageroot = Pdfpage.add_pagetree [page] (Pdf.empty()) in
let pdf = Pdfpage.add_root pageroot [] pdf in
Pdfwrite.pdf_to_file pdf "hello.pdf"

(*
let font =
  Pdf.Dictionary
    [("/Type", Pdf.Name "/Font");
     ("/Subtype", Pdf.Name "/Type1");
     ("/BaseFont", Pdf.Name "/Times-Italic")]
and ops =
  [Pdfops.Op_cm (Pdftransform.matrix_of_transform [Pdftransform.Translate (50., 770.)]);
   Pdfops.Op_BT;
   Pdfops.Op_Tf ("/F0", 36.);
   Pdfops.Op_Tj "Hello, World!";
   Pdfops.Op_ET]
in
  let page =
    {(Pdfpage.blankpage Pdfpaper.a4) with
        Pdfpage.content = [Pdfops.stream_of_ops ops];
        Pdfpage.resources = Pdf.Dictionary [("/Font", Pdf.Dictionary [("/F0", font)])]}
  in
    let pdf, pageroot = Pdfpage.add_pagetree [page] (Pdf.empty ()) in
      let pdf = Pdfpage.add_root pageroot [] pdf in
        Pdfwrite.pdf_to_file pdf "hello.pdf"
*)
