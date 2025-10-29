"""
AI 서버 테스트 클라이언트
"""

import socket
import json
import os

def test_ai_server():
    """
    AI 서버에 테스트 이미지 전송
    """
    print("=" * 50)
    print("AI 서버 테스트 클라이언트")
    print("=" * 50)
    
    # 1. 테스트 이미지 찾기
    test_paths = [
        'Screw.v3i.folder/test/defected',
        'Screw.v3i.folder/test/Non_defected'
    ]
    
    test_image = None
    for path in test_paths:
        if os.path.exists(path):
            files = [f for f in os.listdir(path) if f.endswith(('.jpg', '.png', '.jpeg'))]
            if files:
                test_image = os.path.join(path, files[0])
                break
    
    if not test_image:
        print("❌ 테스트 이미지를 찾을 수 없습니다!")
        return
    
    print(f"✅ 테스트 이미지: {test_image}")
    
    # 2. 이미지 로드
    with open(test_image, 'rb') as f:
        img_data = f.read()
    
    print(f"📦 이미지 크기: {len(img_data)} bytes")
    
    # 3. 서버 연결
    print("\n🔌 서버 연결 중... (localhost:6666)")
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect(('127.0.0.1', 6666))
        print("✅ 연결 성공!")
    except Exception as e:
        print(f"❌ 연결 실패: {e}")
        print("\n💡 먼저 AI 서버를 실행하세요:")
        print("   python ai_server_classification.py")
        return
    
    # 4. 이미지 전송
    print("\n📤 이미지 전송 중...")
    client.sendall(len(img_data).to_bytes(4, 'little'))
    client.sendall(img_data)
    print("✅ 전송 완료!")
    
    # 5. 결과 수신
    print("\n📥 결과 수신 중...")
    size_data = client.recv(4)
    result_size = int.from_bytes(size_data, 'little')
    
    result_data = b''
    while len(result_data) < result_size:
        chunk = client.recv(min(4096, result_size - len(result_data)))
        if not chunk:
            break
        result_data += chunk
    
    # 6. 결과 파싱
    result = json.loads(result_data.decode('utf-8'))
    
    print("\n" + "=" * 50)
    print("AI 분석 결과:")
    print("=" * 50)
    print(f"성공 여부: {result['success']}")
    print(f"분류: {result['classification']}")
    print(f"결과: {result['result']}")
    print(f"신뢰도: {result['confidence']*100:.1f}%")
    print(f"불량 여부: {'불량' if result['is_defect'] else '정상'}")
    print("=" * 50)
    
    client.close()
    print("\n✅ 테스트 완료!")

if __name__ == '__main__':
    test_ai_server()