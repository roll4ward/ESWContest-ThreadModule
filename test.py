import asyncio
from aiocoap import *
import struct

async def main():
    context = await Context.create_client_context()

    # IPv6 주소와 6000번 포트를 사용하는 CoAP URI (실제 값으로 변경)
    uri = "coap://[fd56:00fd:4e96:857f:2e42:bb39:8b8e:a960]:6000/value"

    # CoAP 요청 생성 (GET 요청 가정)
    request = Message(code=GET, uri=uri)

    # 재전송 비활성화 및 Observe 옵션 제거

    request.opt.observe = 0

    # 필요하다면, payload 추가
    # request.payload = b"Hello, CoAP server!"  # 예시 payload

    try:
        response = await context.request(request).response
    except Exception as e:
        print(f'Failed to fetch resource: {e}')
    else:
        value = struct.unpack('<i', response.payload)[0] 
        print(f'Payload (int): {value}')

if __name__ == "__main__":
    asyncio.run(main())