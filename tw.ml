(* 595x842 *)

type page_state = Empty | Text | EOF;;

type page_description = {
  state: page_state;
  ops  : Pdfops.t list;
  y    : float;
};;

let empty_page_description = {state = Empty; ops = []; y = (842.0 -. 32.0)};;

let rec make_page_description desc =
  if desc.y < 20.0 then desc else
  try
    let line = read_line () in
    make_page_description { desc with
      y = desc.y -. 12.0;
      ops = desc.ops
        @ (match desc.state with Empty -> [Pdfops.Op_Tf ("/F0", 12.0); Pdfops.Op_BT] | _ -> [])
        @ [Pdfops.Op_Tm (Pdftransform.matrix_of_transform
             [Pdftransform.Translate (20.0, desc.y)]);
           Pdfops.Op_Tj line];
      state = Text; }
  with End_of_file -> {desc with state = EOF};;

let rec make_pages () =
  let make_page =
    let desc = make_page_description empty_page_description in
    ( { (Pdfpage.blankpage Pdfpaper.a4) with Pdfpage.content = [Pdfops.stream_of_ops desc.ops] }, desc.state = EOF ) in
  match make_page with
  | (page, true) -> [page]
  | (page, false) -> page :: make_pages ();;

let pdf, pageroot = Pdfpage.add_pagetree (make_pages ()) (Pdf.empty()) in
let pdf = Pdfpage.add_root pageroot [] pdf in
Pdfwrite.pdf_to_file pdf "hello.pdf"

(*
let make_pages = [ { (Pdfpage.blankpage Pdfpaper.a4) with Pdfpage.content = [Pdfops.stream_of_ops []] } ] in
let pdf, pageroot = Pdfpage.add_pagetree make_pages (Pdf.empty()) in
let pdf = Pdfpage.add_root pageroot [] pdf in
Pdfwrite.pdf_to_file pdf "hello.pdf"
*)

(*
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
*)

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
