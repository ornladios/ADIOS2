@0xf1e3703fd89311d1;

struct TSGroup {
  sourceRank @0 : UInt32;
  step @1 :UInt32;

  variables @2 :List(VariableInfo);
  dataBlockSize @3 : UInt64;

  struct VariableInfo {
    union {
       array @0 :ArrayVariableInfo;
       global @1 :GlobalVariableInfo;
    }
  }  	 
  struct GlobalVariableInfo {
    number @0 :UInt32;
    name @11 :Text;
    union {
      int8 @1 : Int8;
      int16 @2 : Int16;
      int32 @3 : Int32;
      int64 @4 : Int64;
      uint8 @5 : UInt8;
      uint16 @6 : UInt16;
      uint32 @7 : UInt32;
      uint64 @8 : UInt64;
      double @9 : Float64;
      float @10 : Float32;
    }
  }

  struct ArrayVariableInfo {
    number @0 :UInt32;
    name @1 :Text;
    type @2 :UInt8;
    shape  @3 :List(UInt32);
    blocks @4 :List(BlocksInfo);
  }
  struct BlocksInfo {
    start  @0 :List(UInt64);
    count  @1 :List(UInt32);
    dataOffset @2 :UInt64;
  }
}
