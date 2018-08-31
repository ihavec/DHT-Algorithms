package main

import (
    "flag"
    "fmt"
    "io/ioutil"
    "os"
    "github.com/klauspost/reedsolomon"
)

var dataShards = flag.Int("data", 4, "Number of shards to split the data into")
var parShards = flag.Int("par", 2, "Number of parity shards")
var outFile = flag.String("out", "", "Alternative output path/file")

func init() {
    flag.Usage = func() {
        fmt.Fprintf(os.Stderr, "Usage of %s:\n", os.Args[0])
        fmt.Fprintf(os.Stderr, "  simple-decoder [-flags] basefile.ext\nDo not add the number to the filename.\n")
        fmt.Fprintf(os.Stderr, "Valid flags:\n")
        flag.PrintDefaults()
    }
}

func main() {
    //命令行参数解析
    flag.Parse()
    args := flag.Args()
    if len(args) != 1 {
        fmt.Fprintf(os.Stderr, "Error: No filenames given\n")
        flag.Usage()
        os.Exit(1)
    }
    fname := args[0]

    //创建新编码器并初始化为dataShards数据分片数和parShards奇偶校验分片数
    enc, err := reedsolomon.New(*dataShards, *parShards)
    checkErr(err)

    //创建分片并加载数据
    shards := make([][]byte, *dataShards+*parShards)
    for i := range shards {
        infn := fmt.Sprintf("%s.%d", fname, i)
        fmt.Println("Opening", infn)
        shards[i], err = ioutil.ReadFile(infn)
        if err != nil {
            fmt.Println("Error reading file", err)
            shards[i] = nil
        }
    }

    //验证分片
    ok, err := enc.Verify(shards)
    if ok {
        fmt.Println("无需重建")
    } else {
        fmt.Println("验证失败,重建数据.")
        err = enc.Reconstruct(shards)
        if err != nil {
            fmt.Println("重建失败 -", err)
            os.Exit(1)
        }
        ok, err = enc.Verify(shards)
        if !ok {
            fmt.Println("重建后验证失败，数据可能已损坏!")
            os.Exit(1)
        }
        checkErr(err)
    }

    // Join the shards and write them
    outfn := *outFile
    if outfn == "" {
        outfn = fname
    }

    fmt.Println("Writing data to", outfn)
    f, err := os.Create(outfn)
    checkErr(err)

    // Join the shards and write the data segment to dst.
    err = enc.Join(f, shards, len(shards[0])**dataShards)
    checkErr(err)
}

func checkErr(err error) {
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error: %s", err.Error())
        os.Exit(2)
    }
}
