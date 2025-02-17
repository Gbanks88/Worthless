import requests
import json

# API endpoint
BASE_URL = "http://localhost:8000"

# Sample documents
sample_documents = [
    # Scholarships
    {
        "doc_type": "scholarship",
        "content": "STEM Excellence Scholarship: $10,000 award for undergraduate students pursuing degrees in Science, Technology, Engineering, or Mathematics. Must maintain 3.5 GPA and demonstrate research experience.",
        "metadata": {
            "amount": 10000,
            "deadline": "2025-05-01",
            "field": "STEM",
            "level": "undergraduate"
        }
    },
    {
        "doc_type": "scholarship",
        "content": "Future Leaders in AI Grant: Supporting graduate students specializing in artificial intelligence and quantum computing. Full tuition coverage plus $25,000 annual stipend.",
        "metadata": {
            "amount": 25000,
            "deadline": "2025-06-15",
            "field": "AI/Quantum",
            "level": "graduate"
        }
    },
    
    # Laws
    {
        "doc_type": "law",
        "content": "Quantum Technology Protection Act of 2024: Regulations governing the development, testing, and deployment of quantum computing systems. Includes security requirements and ethical guidelines.",
        "metadata": {
            "jurisdiction": "Federal",
            "year": 2024,
            "category": "Technology",
            "status": "Active"
        }
    },
    {
        "doc_type": "law",
        "content": "AI Research Data Privacy Law: Requirements for handling personal data in artificial intelligence research, including quantum computing applications. Mandates encryption and consent protocols.",
        "metadata": {
            "jurisdiction": "State",
            "year": 2024,
            "category": "Privacy",
            "status": "Active"
        }
    },
    
    # Business Analysis
    {
        "doc_type": "business",
        "content": "Quantum Computing Market Analysis 2025: Industry growth projection of 32% CAGR. Key players include IBM, Google, and emerging startups. Focus on financial applications and cryptography.",
        "metadata": {
            "sector": "Technology",
            "year": 2025,
            "type": "Market Analysis",
            "confidence": 0.85
        }
    },
    {
        "doc_type": "business",
        "content": "AI-Quantum Integration in Financial Services: Analysis of hybrid AI-quantum solutions in banking. Projected 45% efficiency improvement in risk assessment and trading algorithms.",
        "metadata": {
            "sector": "Finance",
            "year": 2025,
            "type": "Industry Report",
            "confidence": 0.92
        }
    }
]

def add_documents():
    print("Adding sample documents...")
    for doc in sample_documents:
        response = requests.post(f"{BASE_URL}/documents/", json=doc)
        print(f"Added {doc['doc_type']} document: {response.status_code}")

def test_searches():
    # Test cases
    search_queries = [
        {
            "query": "quantum computing scholarships for graduate students",
            "doc_type": "scholarship",
            "top_k": 3
        },
        {
            "query": "AI and quantum computing regulations and privacy laws",
            "doc_type": "law",
            "top_k": 3
        },
        {
            "query": "market analysis of quantum computing in financial sector",
            "doc_type": "business",
            "top_k": 3
        },
        {
            "query": "quantum technology applications and regulations",
            "doc_type": None,  # Search across all types
            "top_k": 5
        }
    ]
    
    print("\nPerforming test searches...")
    for query in search_queries:
        print(f"\nSearching for: {query['query']}")
        response = requests.post(f"{BASE_URL}/search/", json=query)
        results = response.json()["results"]
        
        print(f"Found {len(results)} results:")
        for i, result in enumerate(results, 1):
            print(f"\n{i}. Type: {result['type']}")
            print(f"   Similarity: {result['similarity']:.3f}")
            print(f"   Content: {result['content'][:100]}...")

def analyze_relationships():
    print("\nAnalyzing document relationships...")
    # Analyze the first document's relationships
    response = requests.get(f"{BASE_URL}/analyze/0")
    analysis = response.json()["analysis"]
    
    print("\nDocument Relationship Analysis:")
    print(f"Centrality: {json.dumps(analysis['centrality'], indent=2)}")
    print(f"Number of communities: {len(analysis['communities'])}")

if __name__ == "__main__":
    add_documents()
    test_searches()
    analyze_relationships()
