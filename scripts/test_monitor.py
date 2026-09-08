#!/usr/bin/env python3

from pathlib import Path
from tempfile import TemporaryDirectory

import monitor


def main():
    with TemporaryDirectory() as temporary_directory:
        monitor.LOG = Path(temporary_directory) / 'apollo.log'
        monitor.LOG.write_text(
            '\n'.join([
                'I (1000) CodexVoice: received=243 bytes=3 rate=16000',
                'I (1001) AudioService: played=1 peak=12000 backlog=17',
                'I (1100) Application: << Keepalive reply',
                'I (4000) CodexVoice: received=243 bytes=184 rate=16000',
                'I (4001) AudioService: played=2 peak=12000 backlog=2',
                'I (4100) Application: << Real audio reply',
            ])
        )

        data = monitor.build()
        assert data['turns'][0]['spoken'] is False
        assert data['turns'][0]['audio_frames'] == 0
        assert data['turns'][1]['spoken'] is True
        assert data['turns'][1]['audio_frames'] == 1
        assert data['stats']['audio_frames'] == 1
        assert data['stats']['max_backlog'] == 2
        assert data['stats']['silent'] == 1


if __name__ == '__main__':
    main()
