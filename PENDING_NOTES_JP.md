# 次回リリース向けメモ（直接push分のみ）

PRを経由した変更は自動生成（generate_release_notes）で拾われるため、ここに書く必要はない。
jirogit/dev への直接push（fork限定ドキュメント等）だけをここに1行で書き足す。

リリース時：この内容をドラフトリリース本文にコピーしてから、このファイルを空にする。

---

- M5Stack Unit C6L repeater/room_server: `ARDUINO_USB_CDC_ON_BOOT`/`ARDUINO_USB_MODE`未設定によりUSB管理コンソールが無応答だった問題を修正（PR #3099 kiss_modemと同種の問題）。実機でUSB管理コンソール応答を確認済み。
- upstream v1.17.0 を取り込み（JSON config化、Packet Collision Improvements等）。T1000-E実機でrepeaterとして正常動作・メッシュ参加を確認済みだが、**LBT/CAD輻輳挙動の実地確認は未実施**。Discord等での告知時、「v1.17.0取り込み後、衝突時の挙動に変化がないか気づいたら教えてください」とコミュニティに一言添えて後追いで拾えるようにすること。

