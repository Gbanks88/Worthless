from fastapi import FastAPI, HTTPException
from pydantic import BaseModel
from typing import List, Dict, Any, Optional
from quantum_search_engine import HybridSearchEngine

app = FastAPI(title="Quantum-Enhanced Search API")
search_engine = HybridSearchEngine()

class Document(BaseModel):
    doc_type: str  # 'scholarship', 'law', or 'business'
    content: str
    metadata: Dict[str, Any]

class SearchQuery(BaseModel):
    query: str
    doc_type: Optional[str] = None
    top_k: int = 5

@app.post("/documents/")
async def add_document(document: Document):
    try:
        search_engine.add_document(
            doc_type=document.doc_type,
            content=document.content,
            metadata=document.metadata
        )
        return {"status": "success", "message": "Document added successfully"}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.post("/search/")
async def search(query: SearchQuery):
    try:
        results = search_engine.search(
            query=query.query,
            doc_type=query.doc_type,
            top_k=query.top_k
        )
        return {"results": results}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@app.get("/analyze/{doc_id}")
async def analyze_relationships(doc_id: int):
    try:
        analysis = search_engine.analyze_relationships(doc_id)
        if not analysis:
            raise HTTPException(status_code=404, detail="Document not found")
        return {"analysis": analysis}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)
