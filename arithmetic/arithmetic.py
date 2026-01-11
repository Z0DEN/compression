import os
import struct
import sys
import time
from collections import Counter


class ArithmeticCoder:
    def __init__(self):
        # Работаем в 32-битном целочисленном "интервале" [0, 2^32 - 1]
        self.MAX_RANGE = 1 << 32

    def build_cumulative(self, freq):
        # Строим отсортированный список символов и их кумулятивные частоты.
        # cumulative_freq[i] - сумма частот символов с индексами < i.
        symbols = sorted(freq.keys())
        cumulative_freq = [0]
        for s in symbols:
            cumulative_freq.append(cumulative_freq[-1] + freq[s])
        return symbols, cumulative_freq

    def encode_file(self, input_file, output_file):
        if not os.path.exists(input_file):
            script_dir = os.path.dirname(__file__)
            alt_path = os.path.join(script_dir, input_file)
            if os.path.exists(alt_path):
                input_file = alt_path
            else:
                print(f"Ошибка: Файл {input_file} не найден!")
                return False

        try:
            with open(input_file, 'r', encoding='utf-8') as f:
                text = f.read()
        except Exception as e:
            print(f"Ошибка при чтении {input_file}: {e}")
            return False

        if not text:
            print("Файл пуст")
            return False

        # Подсчитываем частоты символов во всём тексте.
        freq = Counter(text)
        symbols, cumulative_freq = self.build_cumulative(freq)
        total = cumulative_freq[-1]  # суммарное количество символов

        class BitWriter:
            # Простой буфер для побитовой записи.
            def __init__(self):
                self.buf = 0      # текущий неполный байт
                self.n = 0        # сколько бит уже записано в buf
                self.bytes = bytearray()

            def put_bit(self, b):
                # Сдвигаем buf и дописываем младший бит.
                self.buf = (self.buf << 1) | (1 if b else 0)
                self.n += 1
                # Когда набралось 8 бит - сохраняем байт.
                if self.n == 8:
                    self.bytes.append(self.buf)
                    self.buf = 0
                    self.n = 0

            def put_bits(self, b, count):
                # Записываем count старших бит числа b.
                for i in range(count - 1, -1, -1):
                    self.put_bit((b >> i) & 1)

            def flush(self):
                # Если остались недописанные биты - дополняем нулями до байта.
                if self.n > 0:
                    self.buf <<= (8 - self.n)
                    self.bytes.append(self.buf)

        # Константы для деления интервала на половины и четверти:
        # [0, HALF), [HALF, 2*HALF) и четверти.
        MASK = (1 << 32) - 1
        HALF = 1 << 31
        QUARTER = 1 << 30

        # Начальный интервал: [low, high] = весь диапазон.
        low = 0
        high = MASK
        # pending_bits - сколько раз интервал попал между 1/4 и 3/4 (E3 зона).
        # В этом случае биты нельзя сразу вывести в поток, их надо отложить.
        pending_bits = 0

        writer = BitWriter()
        # Быстрый переход от символа к его индексу в массиве cumulative_freq.
        sym_to_idx = {s: i for i, s in enumerate(symbols)}

        for ch in text:
            i = sym_to_idx[ch]
            range_ = high - low + 1
            # Делим текущий интервал пропорционально частотам и берём подинтервал символа.
            high = low + (range_ * cumulative_freq[i + 1]) // total - 1
            low = low + (range_ * cumulative_freq[i]) // total

            # Нормализация: пока интервал целиком лежит в нижней половине, верхней половине или в середине -
            # сдвигаем его и при необходимости пишем биты.
            while True:
                if high < HALF:
                    # Весь интервал в [0, 1/2):
                    # общий старший бит - 0, можно вывести его.
                    writer.put_bit(0)
                    # Все отложенные средние случаи (E3) превращаются в 1.
                    for _ in range(pending_bits):
                        writer.put_bit(1)
                    pending_bits = 0
                    # Сдвигаем интервал влево, чтобы использовать весь диапазон снова.
                    low = (low << 1) & MASK
                    high = ((high << 1) & MASK) | 1
                elif low >= HALF:
                    # Весь интервал в [1/2, 1):
                    # общий старший бит - 1.
                    writer.put_bit(1)
                    # Все отложенные E3-случаи превращаются в 0.
                    for _ in range(pending_bits):
                        writer.put_bit(0)
                    pending_bits = 0
                    # Вычитаем HALF и снова растягиваем интервал.
                    low = ((low - HALF) << 1) & MASK
                    high = (((high - HALF) << 1) & MASK) | 1
                elif low >= QUARTER and high < 3 * QUARTER:
                    # Интервал попал в середину [1/4, 3/4):
                    # ещё рано выводить бит, только запоминаем что потом надо будет дописать инвертированный бит.
                    pending_bits += 1
                    low = ((low - QUARTER) << 1) & MASK
                    high = (((high - QUARTER) << 1) & MASK) | 1
                else:
                    # Интервал не попадает ни в один из специальных случаев - выходим.
                    break

        # После кодирования всех символов надо сделать еще один шаг нормализации:
        # выбираем бит по положению low и дописываем накопленные биты.
        pending_bits += 1
        if low < QUARTER:
            # Если интервал ближе к 0 то пишем 0, а за ним единицы по длине pending_bits.
            writer.put_bit(0)
            for _ in range(pending_bits):
                writer.put_bit(1)
        else:
            # Интервал ближе к 1 - пишем 1, а за ним нули столько же, сколько бит в pending_bits.
            writer.put_bit(1)
            for _ in range(pending_bits):
                writer.put_bit(0)

        writer.flush()
        bit_length = len(writer.bytes) * 8

        with open(output_file, 'wb') as f:
            # Сохраняем длину текста и модель (частоты), чтобы декодер мог восстановить тот же самый интервал.
            f.write(struct.pack('I', len(text)))
            f.write(struct.pack('H', len(freq)))
            # Таблица частот: для каждого символа - длина в байтах, байты символа, частота.
            for ch, fr in freq.items():
                b = ch.encode('utf-8')
                f.write(struct.pack('H', len(b)))
                f.write(b)
                f.write(struct.pack('I', fr))
            # Сохраняем длину битового потока и сами байты.
            f.write(struct.pack('I', bit_length))
            f.write(bytes(writer.bytes))

        return True

    def decode_file(self, input_file, output_file):
        if not os.path.exists(input_file):
            script_dir = os.path.dirname(__file__)
            alt_path = os.path.join(script_dir, input_file)
            if os.path.exists(alt_path):
                input_file = alt_path
            else:
                print(f"Ошибка: Файл {input_file} не найден!")
                return False

        try:
            with open(input_file, 'rb') as f:
                text_len = struct.unpack('I', f.read(4))[0]
                sym_count = struct.unpack('H', f.read(2))[0]
                freq = {}
                # Восстанавливаем таблицу частот (модель).
                for _ in range(sym_count):
                    blen = struct.unpack('H', f.read(2))[0]
                    b = f.read(blen)
                    fr = struct.unpack('I', f.read(4))[0]
                    freq[b.decode('utf-8')] = fr
                # Длина битового потока и собственно данные.
                bit_length = struct.unpack('I', f.read(4))[0]
                payload = f.read()
        except Exception as e:
            print(f"Ошибка при чтении {input_file}: {e}")
            return False

        symbols, cumulative_freq = self.build_cumulative(freq)
        total = cumulative_freq[-1]

        class BitReader:
            # Обратный класс для побитового чтения из массива байт.
            def __init__(self, data, bitlen):
                self.data = data
                self.bitlen = bitlen
                self.pos = 0  # текущая позиция в битах

            def next_bit(self):
                # Если биты закончились, возвращаем 0, чтобы не получить ошибку.
                if self.pos >= self.bitlen:
                    return 0
                byte_idx = self.pos // 8
                bit_idx = 7 - (self.pos % 8)
                b = (self.data[byte_idx] >> bit_idx) & 1
                self.pos += 1
                return b

        br = BitReader(payload, bit_length)

        MASK = (1 << 32) - 1
        HALF = 1 << 31
        QUARTER = 1 << 30

        # Инициализируем регистр value первыми 32 битами закодированного потока.
        value = 0
        for _ in range(32):
            value = (value << 1) | br.next_bit()

        low = 0
        high = MASK

        result = []

        # Декодируем ровно text_len символов.
        for _ in range(text_len):
            range_ = high - low + 1
            # Переводим текущий value в индекс на отрезке [0, total).
            scaled = ((value - low + 1) * total - 1) // range_

            # Находим, в какой кумулятивный интервал попал scaled.
            # Это и есть восстанавливаемый символ.
            # При большом алфавите здесь лучше сделать бинарный поиск.
            for i in range(len(symbols)):
                if cumulative_freq[i] <= scaled < cumulative_freq[i + 1]:
                    ch = symbols[i]
                    break

            result.append(ch)

            # Обновляем интервал под этот символ.
            high = low + (range_ * cumulative_freq[i + 1]) // total - 1
            low = low + (range_ * cumulative_freq[i]) // total

            # Нормализация low/high/value по правилам E1/E2/E3, чтобы декодер оставался синхронен с энкодером.
            while True:
                if high < HALF:
                    # Интервал в нижней половине: просто сдвигаем всё влево.
                    pass
                elif low >= HALF:
                    # Интервал в верхней половине: вычитаем HALF и сдвигаем.
                    low -= HALF
                    high -= HALF
                    value -= HALF
                elif low >= QUARTER and high < 3 * QUARTER:
                    # Интервал в середине: вычитаем QUARTER и сдвигаем.
                    low -= QUARTER
                    high -= QUARTER
                    value -= QUARTER
                else:
                    break

                # После любой из трёх ситуаций растягиваем интервал обратно.
                low = (low << 1) & MASK
                high = ((high << 1) & MASK) | 1
                # подтягиваем следующий бит из потока в младший разряд value.
                value = ((value << 1) & MASK) | br.next_bit()

        with open(output_file, 'w', encoding='utf-8') as f:
            f.write(''.join(result))

        return True


def main():

        if len(sys.argv) < 2:
            print("Использование:")
            print("  python lab2.py encode <input.txt> <output.arith>")
            print("  python lab2.py decode <input.arith> <output.txt>")
            return

        mode = sys.argv[1].lower()
        coder = ArithmeticCoder()

        if mode == "encode":
            if len(sys.argv) != 4:
                print("Формат: python lab2.py encode <input.txt> <output.arith>")
                return
            input_file = sys.argv[2]
            output_file = sys.argv[3]

            start = time.time()
            ok = coder.encode_file(input_file, output_file)
            elapsed = time.time() - start
            if ok:
                # Считаем степень сжатия по размеру файлов.
                try:
                    orig_size = os.path.getsize(input_file)
                    comp_size = os.path.getsize(output_file)
                    if comp_size > 0:
                        ratio = orig_size / comp_size
                        print("Кодирование завершено.")
                        print(f"Исходный размер: {orig_size} байт")
                        print(f"Сжатый размер:   {comp_size} байт")
                        print(f"Степень сжатия:  {ratio:.3f}")
                    else:
                        print("Кодирование завершено, но размер архива 0 байт.")
                except OSError as e:
                    print(f"Невозможно получить размеры файлов: {e}")
                print(f"Время: {elapsed:.3f} сек")
        elif mode == "decode":
            if len(sys.argv) != 4:
                print("Формат: python lab2.py decode <input.arith> <output.txt>")
                return
            input_file = sys.argv[2]
            output_file = sys.argv[3]

            start = time.time()
            ok = coder.decode_file(input_file, output_file)
            elapsed = time.time() - start
            if ok:
                print("Декодирование завершено.")
                print(f"Время: {elapsed:.3f} сек")
        else:
            print("Неизвестный режим. Используйте encode / decode / freq.")


if __name__ == "__main__":
    main()
