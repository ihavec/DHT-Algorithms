package main

import (
    "flag"
    "fmt"
    "io/ioutil"
    "os"
    "path/filepath"
    "github.com/klauspost/reedsolomon"
)

var dataShards = flag.Int("data", 4, "Number of shards to split the data into, must be below 257.")
var parShards = flag.Int("par", 2, "Number of parity shards")
var outDir = flag.String("out", "", "Alternative output directory")

func init() {
    flag.Usage = func() {
        fmt.Fprintf(os.Stderr, "Usage of %s:\n", os.Args[0])
        fmt.Fprintf(os.Stderr, "  simple-encoder [-flags] filename.ext\n\n")
        fmt.Fprintf(os.Stderr, "Valid flags:\n")
        flag.PrintDefaults()
    }
}

func main() {
    //命令行参数解析
    flag.Parse()
    args := flag.Args()
    if len(args) != 1 {
        fmt.Fprintf(os.Stderr, "Error: No input filename given\n")
        flag.Usage()
        os.Exit(1)
    }
    if *dataShards > 257 {
        fmt.Fprintf(os.Stderr, "Error: Too many data shards\n")
        os.Exit(1)
    }
    fname := args[0]

    //创建新编码器并初始化为dataShards数据分片数和parShards奇偶校验分片数
    enc, err := reedsolomon.New(*dataShards, *parShards)
    checkErr(err)

    fmt.Println("Opening", fname)
    b, err := ioutil.ReadFile(fname)
    checkErr(err)

    //将文件拆分为大小相等的分片,并创建空的奇偶校验分片.
    shards, err := enc.Split(b)
    checkErr(err)
    fmt.Printf("File split into %d data+parity shards with %d bytes/shard.\n", len(shards), len(shards[0]))

    //奇偶校验编码
    err = enc.Encode(shards)
    checkErr(err)

    //输出到分片文件
    dir, file := filepath.Split(fname)
    if *outDir != "" {
        dir = *outDir
    }
    for i, shard := range shards {
        outfn := fmt.Sprintf("%s.%d", file, i)

        fmt.Println("Writing to", outfn)
        err = ioutil.WriteFile(filepath.Join(dir, outfn), shard, os.ModePerm)
        checkErr(err)
    }
}

func checkErr(err error) {
    if err != nil {
        fmt.Fprintf(os.Stderr, "Error: %s", err.Error())
        os.Exit(2)
    }
}
