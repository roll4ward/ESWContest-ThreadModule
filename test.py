import asyncio
from aiocoap import *
import struct

async def main():
    context = await Context.create_client_context()

    # IPv6 주소와 6000번 포트를 사용하는 CoAP URI (실제 값으로 변경)
    uri = "coap://[fd56:00fd:4e96:857f:a787:4fe8:9243:c262]:6000/reset"

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
        print(f'Payload (str): {response.payload}')

if __name__ == "__main__":
    asyncio.run(main())