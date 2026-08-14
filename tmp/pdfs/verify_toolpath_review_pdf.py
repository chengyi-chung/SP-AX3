from pathlib import Path
import fitz
import pdfplumber

root = Path(r"D:\Git Repository\chengyi-chung\SP-AX3")
pdf_path = root / "output" / "pdf" / "ToolPath_Flow_Design_Review.pdf"
render_dir = root / "tmp" / "pdfs" / "rendered_toolpath_review"
render_dir.mkdir(parents=True, exist_ok=True)

doc = fitz.open(pdf_path)
for i, page in enumerate(doc):
    pix = page.get_pixmap(matrix=fitz.Matrix(1.5, 1.5), alpha=False)
    pix.save(render_dir / f"page-{i + 1:02d}.png")

with pdfplumber.open(pdf_path) as pdf:
    text = "\n".join((page.extract_text() or "") for page in pdf.pages)
    required = [
        "端到端資料流", "ToolPathType 分派", "HMI / Modbus 輸出",
        "設計審查發現", "建議驗收案例"
    ]
    missing = [item for item in required if item not in text]
    print(f"pages={len(pdf.pages)} chars={len(text)} missing={missing}")
print(render_dir)
