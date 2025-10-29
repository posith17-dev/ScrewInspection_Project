"""
Qt 서버 테스트 클라이언트
MFC 역할 시뮬레이션
"""

import socket
import json
import os

def test_qt_server():
    """
    Qt 서버에 이미지 전송하고 결과 수신
    """
    print("=" * 50)
    print("Qt 서버 테스트 (MFC 시뮬레이션)")
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
    
    # 3. Qt 서버 연결
    print("\n🔌 Qt 서버 연결 중... (localhost:5555)")
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.connect(('127.0.0.1', 5555))
        print("✅ Qt 서버 연결 성공!")
    except Exception as e:
        print(f"❌ 연결 실패: {e}")
        print("\n💡 Qt 서버가 실행 중인지 확인하세요:")
        print("   ./bin/QtServer.exe")
        return
    
    # 4. 이미지 전송
    print("\n📤 이미지 전송 중...")
    client.sendall(len(img_data).to_bytes(4, 'little'))
    client.sendall(img_data)
    print("✅ 전송 완료!")
    
    # 5. 결과 수신
    print("\n📥 결과 수신 중...")
    print("   (Python AI → Qt 서버 → 여기)")
    
    try:
        size_data = client.recv(4)
        if not size_data:
            print("❌ 결과 수신 실패 (연결 종료됨)")
            return
        
        result_size = int.from_bytes(size_data, 'little')
        print(f"   결과 크기: {result_size} bytes")
        
        result_data = b''
        while len(result_data) < result_size:
            chunk = client.recv(min(4096, result_size - len(result_data)))
            if not chunk:
                break
            result_data += chunk
        
        # 6. 결과 파싱
        result = json.loads(result_data.decode('utf-8'))
        
        print("\n" + "=" * 50)
        print("최종 결과 (MFC가 받을 데이터):")
        print("=" * 50)
        print(f"성공 여부: {result.get('success', 'N/A')}")
        print(f"분류: {result.get('classification', 'N/A')}")
        print(f"결과: {result.get('result', 'N/A')}")
        print(f"신뢰도: {result.get('confidence', 0)*100:.1f}%")
        print(f"불량 여부: {result.get('is_defect', 'N/A')}")
        print("=" * 50)
        
        print("\n✅ 전체 통신 흐름 테스트 성공!")
        print("\n흐름:")
        print("  [테스트 클라이언트] → [Qt 서버] → [Python AI]")
        print("                    ← [Qt 서버] ← [Python AI]")
        
    except socket.timeout:
        print("❌ 타임아웃: 결과를 받지 못했습니다")
        print("   Python AI 서버가 실행 중인지 확인하세요")
    except Exception as e:
        print(f"❌ 에러: {e}")
    finally:
        client.close()

if __name__ == '__main__':
    test_qt_server()
